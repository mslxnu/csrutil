/*
 * csr.c — Core SIP read/write logic.
 *
 * This file implements the high-level csrutil API:
 *   csrutil_status()     — read current SIP state
 *   csrutil_set_flags()  — modify SIP flags
 *   csrutil_disable()    — convenience: disable SIP entirely
 *   csrutil_enable()     — convenience: enable SIP fully
 *   csrutil_reset()      — convenience: restore factory defaults
 *
 * Internally it calls into libbootpolicy (via bootpolicy.h) for
 * LocalPolicy access and into acm.h for credential management.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include "csrutil.h"
#include "bootpolicy.h"
#include "acm.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <unistd.h>

/* ── Internal helpers ─────────────────────────────────────────────── */

static bp_cfref_t default_volume_path(void)
{
    return bp_default_policy_volume_path();
}

static bp_cfref_t default_sfr_manifest_path(void)
{
    return bp_default_sfr_manifest_path();
}

static int resolve_boot_uuid(bp_cfuuid_t *uuid_out)
{
    bool has = false;
    bp_cfref_t vol = default_volume_path();
    if (!vol)
        return CSRUTIL_ERR_READ_FAILED;
    int  rc  = bootpolicy_volume_has_local_policy(vol, uuid_out, &has);
    if (rc != 0)
        return CSRUTIL_ERR_READ_FAILED;
    if (!has)    return CSRUTIL_ERR_NO_POLICY;
    return CSRUTIL_OK;
}

/* ── Kernel syscall (from System.framework) ───────────────────────── */

extern int csr_get_active_config(uint32_t *config);

/* ── Platform detection ───────────────────────────────────────────── */

bool csrutil_is_apple_silicon(void)
{
    struct utsname u;
    if (uname(&u) != 0) return false;
    return strcmp(u.machine, "arm64") == 0;
}

bool csrutil_is_recovery(void)
{
    char buf[16] = {0};
    size_t len = sizeof(buf);
    int rc = sysctlbyname("kern.osvariant_status", buf, &len, NULL, 0);
    if (rc != 0) {
        return access("/.IAmRecoveryEnvironment", F_OK) == 0;
    }
    return (buf[0] & 1) != 0;
}

/* ── Error strings ────────────────────────────────────────────────── */

const char *csrutil_strerror(int error)
{
    switch (error) {
    case CSRUTIL_OK:                 return "success";
    case CSRUTIL_ERR_NOT_ROOT:       return "must be run as root (sudo)";
    case CSRUTIL_ERR_NOT_ARM64:      return "not Apple Silicon (no LocalPolicy)";
    case CSRUTIL_ERR_NO_POLICY:      return "boot volume has no local policy";
    case CSRUTIL_ERR_LIB_NOT_LOADED: return "libbootpolicy.dylib not loaded";
    case CSRUTIL_ERR_ACM_NOT_LOADED: return "ACM subsystem not loaded (LocalAuthenticationCore)";
    case CSRUTIL_ERR_LIB_SYMBOL:     return "missing symbol in libbootpolicy";
    case CSRUTIL_ERR_AUTH_FAILED:    return "authentication failed (wrong password?)";
    case CSRUTIL_ERR_NONCE_BEGIN:    return "nonce_begin() failed";
    case CSRUTIL_ERR_NONCE_END:      return "nonce_end() failed — policy not committed";
    case CSRUTIL_ERR_WRITE_FAILED:   return "failed to write tag/sip flags to LocalPolicy";
    case CSRUTIL_ERR_READ_FAILED:    return "failed to read tag/sip flags from LocalPolicy";
    case CSRUTIL_ERR_PLATFORM:       return "unsupported platform";
    case CSRUTIL_ERR_RECOVERY_ONLY:  return "this command is only available in Recovery OS";
    default:                         return "unknown error";
    }
}

/* ── csrutil_status() ─────────────────────────────────────────────── */

int csrutil_status(csrutil_state_t *state)
{
    if (!state) return CSRUTIL_ERR_READ_FAILED;

    memset(state, 0, sizeof(*state));

    /* Read the raw CSR bitmask via the kernel syscall. */
    int rc = csr_get_active_config(&state->csr_config);
    if (rc != 0) return CSRUTIL_ERR_READ_FAILED;

    /* Derive the individual restriction booleans from the bitmask. */
    state->kext_restricted   = !(state->csr_config & CSR_ALLOW_UNTRUSTED_KEXTS);
    state->fs_restricted     = !(state->csr_config & CSR_ALLOW_UNRESTRICTED_FS);
    state->debug_restricted  = !(state->csr_config & CSR_ALLOW_TASK_FOR_PID);
    state->dtrace_restricted = !(state->csr_config & CSR_ALLOW_UNRESTRICTED_DTRACE);
    state->nvram_restricted  = !(state->csr_config & CSR_ALLOW_UNRESTRICTED_NVRAM);
    state->kernel_debug      = !(state->csr_config & CSR_ALLOW_KERNEL_DEBUGGER);

    /* Derived convenience booleans. */
    state->boot_arg_filter  = state->nvram_restricted;
    state->kext_loading     = (state->csr_config & CSR_ALLOW_UNTRUSTED_KEXTS) != 0;
    state->apple_internal   = (state->csr_config & CSR_ALLOW_APPLE_INTERNAL) != 0;
    state->research_guests  = (state->csr_config & CSR_ALLOW_RESEARCH_GUESTS) != 0;

    /* Try to get additional info from libbootpolicy. */
    state->security_mode_name = "Full";
    state->security_mode = 0;
    if (bp_load() == 0) {
        bp_cfuuid_t uuid = NULL;
        bp_cfref_t vol = default_volume_path();
        if (vol && resolve_boot_uuid(&uuid) == CSRUTIL_OK) {
            int mode = 0;
            if (bootpolicy_get_security_mode(vol, &mode) == 0) {
                state->security_mode = mode;
                switch (mode) {
                case 0:  state->security_mode_name = "Full";       break;
                case 1:  state->security_mode_name = "Reduced";    break;
                case 2:  state->security_mode_name = "Permissive"; break;
                default: state->security_mode_name = "Unknown";    break;
                }
            }
        }
    }

    return CSRUTIL_OK;
}

/* ── csrutil_set_flags() ──────────────────────────────────────────── */

int csrutil_set_flags(uint32_t flags_to_set,
                      uint32_t flags_to_clear,
                      const char *username,
                      const char *password)
{
    if (geteuid() != 0)
        return CSRUTIL_ERR_NOT_ROOT;
    if (!csrutil_is_apple_silicon())
        return CSRUTIL_ERR_NOT_ARM64;

    if (bp_load() != 0)
        return CSRUTIL_ERR_LIB_NOT_LOADED;
    if (acm_load() != 0)
        return CSRUTIL_ERR_ACM_NOT_LOADED;

    /* Authenticate. */
    int auth_err = 0;
    acm_context_t ctx = password
        ? acm_authenticate_with_password(username, password, &auth_err)
        : acm_authenticate_interactive(username, &auth_err);

    if (!ctx) {
        fprintf(stderr, "csrutil: authentication failed (acm error %d)\n",
                auth_err);
        fprintf(stderr, "csrutil: SIP modification requires Apple's "
                "code-signed csrutil binary.\n"
                "csrutil: csrutil cannot acquire the necessary "
                "entitlements on macOS 26.\n");
        return CSRUTIL_ERR_AUTH_FAILED;
    }

    /* Read current config. */
    bp_cfref_t vol = default_volume_path();
    bp_cfref_t sfr = default_sfr_manifest_path();
    bp_cfuuid_t uuid = NULL;
    int rc = resolve_boot_uuid(&uuid);
    if (rc != 0) {
        acm_context_delete(ctx);
        return rc;
    }

    uint32_t current = 0;
    rc = bootpolicy_get_sip_flags(vol, uuid, &current);
    if (rc != 0) {
        fprintf(stderr, "csrutil: failed to read current SIP flags: %d\n", rc);
        acm_context_delete(ctx);
        return CSRUTIL_ERR_READ_FAILED;
    }

    /* Compute new flags: set bits, then clear bits. */
    uint32_t new_flags = (current | flags_to_set) & ~flags_to_clear;
    new_flags &= CSR_VALID_FLAGS;

    if (new_flags == current) {
        fprintf(stderr, "csrutil: configuration unchanged (0x%08x)\n",
                current);
        acm_context_delete(ctx);
        return CSRUTIL_OK;
    }

    /* Begin nonce transaction. */
    rc = bootpolicy_update_local_policy_nonce_begin();
    if (rc != 0) {
        fprintf(stderr, "csrutil: failed to generate nonce: %s\n",
                bootpolicy_error_to_string(rc));
        acm_context_delete(ctx);
        return CSRUTIL_ERR_NONCE_BEGIN;
    }

    /* Update SIP flags.  Mask = all valid bits. */
    rc = bootpolicy_update_sip_flags(vol, sfr,
                                     new_flags, CSR_VALID_FLAGS);
    if (rc != 0) {
        fprintf(stderr, "csrutil: failed to update SIP flags: %s\n",
                bootpolicy_error_to_string(rc));
        bootpolicy_update_local_policy_nonce_reset(vol);
        acm_context_delete(ctx);
        return CSRUTIL_ERR_WRITE_FAILED;
    }

    /* Commit: end the nonce transaction. */
    rc = bootpolicy_update_local_policy_nonce_end(vol, sfr, ctx, NULL);
    if (rc != 0) {
        fprintf(stderr, "csrutil: failed to commit policy: %s\n",
                bootpolicy_error_to_string(rc));
        bootpolicy_update_local_policy_nonce_reset(vol);
        acm_context_delete(ctx);
        return CSRUTIL_ERR_NONCE_END;
    }

    acm_context_delete(ctx);

    fprintf(stderr, "csrutil: System Integrity Protection configuration "
            "changed (0x%08x → 0x%08x).\n", current, new_flags);
    fprintf(stderr, "csrutil: A reboot is required for changes to take effect.\n");
    return CSRUTIL_OK;
}

int csrutil_disable(const char *username, const char *password)
{
    fprintf(stderr, "csrutil: disabling System Integrity Protection.\n");
    return csrutil_set_flags(CSR_SIP_DISABLE_FLAGS, 0, username, password);
}

int csrutil_enable(const char *username, const char *password)
{
    fprintf(stderr, "csrutil: enabling System Integrity Protection.\n");
    return csrutil_set_flags(0, CSR_SIP_DISABLE_FLAGS, username, password);
}

int csrutil_reset(const char *username, const char *password)
{
    return csrutil_enable(username, password);
}

/* ── Flag metadata table ──────────────────────────────────────────── */

const csr_flag_info_t csr_flag_table[] = {
    { CSR_ALLOW_UNTRUSTED_KEXTS,          "Kext Signing",
      "3rd-party kext loading",           "kext" },
    { CSR_ALLOW_UNRESTRICTED_FS,          "Filesystem Protections",
      "filesystem write protections",     "fs" },
    { CSR_ALLOW_TASK_FOR_PID,             "Debugging Restrictions",
      "task_for_pid and debugging",       "debug" },
    { CSR_ALLOW_KERNEL_DEBUGGER,          "Kernel Debugging Restrictions",
      "kernel debugger access",           NULL },
    { CSR_ALLOW_APPLE_INTERNAL,           "Apple Internal",
      "Apple Internal flag",              NULL },
    { CSR_ALLOW_UNRESTRICTED_DTRACE,      "DTrace Restrictions",
      "dtrace probe restrictions",        "dtrace" },
    { CSR_ALLOW_UNRESTRICTED_NVRAM,       "NVRAM Protections",
      "NVRAM write protections",          "nvram" },
    { CSR_ALLOW_DEVICE_CONFIGURATION,     "Device Configuration",
      "device configuration management",  NULL },
    { CSR_ALLOW_ANY_RECOVERY_OS,          "BaseSystem Verification",
      "recovery OS base system verification", "basesystem" },
    { CSR_ALLOW_UNAPPROVED_KEXTS,         "Unapproved Kexts Restrictions",
      "unapproved kext loading",          NULL },
    { CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE, "Executable Policy",
      "executable policy override",       NULL },
    { CSR_ALLOW_AUTHENTICATED_ROOT_REQUIREMENT, "Authenticated Root Requirement",
      "authenticated root requirement",   NULL },
    { CSR_ALLOW_RESEARCH_GUESTS,          "Research Guests",
      "research guest access",            NULL },
};

const size_t csr_flag_table_count =
    sizeof(csr_flag_table) / sizeof(csr_flag_table[0]);

const csr_flag_info_t *csr_flag_lookup_by_arg(const char *arg)
{
    if (!arg) return NULL;
    for (size_t i = 0; i < csr_flag_table_count; i++) {
        if (csr_flag_table[i].csrutil_arg &&
            strcmp(csr_flag_table[i].csrutil_arg, arg) == 0)
            return &csr_flag_table[i];
    }
    return NULL;
}

const csr_flag_info_t *csr_flag_lookup_by_bit(uint32_t bit)
{
    for (size_t i = 0; i < csr_flag_table_count; i++) {
        if (csr_flag_table[i].bit == bit)
            return &csr_flag_table[i];
    }
    return NULL;
}
