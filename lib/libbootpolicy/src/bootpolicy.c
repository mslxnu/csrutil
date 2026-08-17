/*
 * bootpolicy.c — Dynamic loader for libbootpolicy.dylib.
 *
 * libbootpolicy lives in the dyld shared cache and has no standalone file
 * on disk.  We dlopen() it by path (dyld resolves from the cache) and
 * resolve every symbol we need via dlsym().  If any symbol is missing
 * (older macOS, different build) we fail early with a clear error.
 *
 * All bootpolicy_* wrappers in this file follow the same pattern:
 *   1. Check that bp_handle is valid.
 *   2. Forward to the resolved function pointer.
 *   3. Return the result.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include "bootpolicy.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

/* ── dylib handle ─────────────────────────────────────────────────── */

static void *bp_handle = NULL;

/* The canonical path that dyld resolves from the shared cache. */
#define LIBBOOTPOLICY_PATH "/usr/lib/libbootpolicy.dylib"

/* ── Function pointer table ───────────────────────────────────────── */

#define BP_FUNC(ret, name, ...) \
    typedef ret (*name##_fn)(__VA_ARGS__); \
    static name##_fn name##_ptr = NULL;

/* Logging */
BP_FUNC(void, bp_set_log_function, bp_log_fn fn)

/* Error */
BP_FUNC(const char *, bp_error_to_string, int err)

/* OS type */
BP_FUNC(int, bp_get_current_os_type, int *out)
BP_FUNC(int, bp_get_current_os_type_restrictions_override_status, bool *out)
BP_FUNC(bool, bp_has_local_policy_support, void)
BP_FUNC(const char *, bp_os_type_to_string, int t)

/* Volume / policy existence */
BP_FUNC(int, bp_volume_has_local_policy, bp_cfref_t v, bp_cfuuid_t *u, bool *h)
BP_FUNC(int, bp_get_local_policy, bp_cfref_t v, bp_cfuuid_t u, void **d, size_t *s)

/* Boolean tags */
BP_FUNC(int, bp_get_local_policy_boolean_tag,
        bp_cfref_t v, bp_cfuuid_t u, const char *t, bool *o)
BP_FUNC(int, bp_update_local_policy_boolean_tag,
        bp_cfref_t v, bp_cfuuid_t u, bp_cfref_t s,
        const char *t, uint32_t id, bool val, void *ctx)

/* Integer tags */
BP_FUNC(int, bp_get_local_policy_integer_tag,
        bp_cfref_t v, bp_cfuuid_t u, const char *t, int64_t *o)

/* SIP flags */
BP_FUNC(int, bp_get_sip_flags, bp_cfref_t v, bp_cfuuid_t u, uint32_t *f)
BP_FUNC(int, bp_update_sip_flags, bp_cfref_t v, bp_cfref_t s,
        uint32_t f, uint64_t e)
BP_FUNC(int, bp_remove_sip_flags, bp_cfref_t v, bp_cfuuid_t u)

/* Pairing */
BP_FUNC(int, bp_get_local_policy_pairing_status,
        bp_cfref_t v, bp_cfuuid_t u, bool s, bool *o)
BP_FUNC(int, bp_verify_local_policy_pairing,
        bp_cfref_t v, bp_cfref_t r, bp_cfuuid_t u, bp_cfuuid_t rec, bool *o)

/* Nonce lifecycle */
BP_FUNC(int, bp_update_local_policy_nonce_begin, void)
BP_FUNC(int, bp_update_local_policy_nonce_end,
        bp_cfref_t v, bp_cfref_t s, void *c, bp_cfref_t t)
BP_FUNC(int, bp_update_local_policy_nonce_reset, bp_cfref_t v)

/* Security mode */
BP_FUNC(int, bp_get_security_mode, bp_cfref_t v, int *m)

/* Nonce digests */
BP_FUNC(int, bp_get_blessed_local_policy_nonce_digest, void *d)
BP_FUNC(int, bp_get_proposed_local_policy_nonce_digest, void *d)

#undef BP_FUNC

/* ── Global default value pointers (resolved via dlsym) ────────────── */

static bp_cfref_t  *g_default_volume_path_ptr  = NULL;
static bp_cfref_t  *g_default_sfr_path_ptr     = NULL;
static bp_cfuuid_t *g_root_volume_uuid_ptr     = NULL;

/* ── Symbol resolution helper ─────────────────────────────────────── */

#define RESOLVE(var, name) \
    do { \
        var##_ptr = (__typeof__(var##_ptr))dlsym(bp_handle, #name); \
        if (!var##_ptr) { \
            fprintf(stderr, "libbootpolicy: missing symbol: %s\n", #name); \
            return -1; \
        } \
    } while (0)

/* ── Bootstrap ────────────────────────────────────────────────────── */

int bp_load(void)
{
    if (bp_handle)
        return 0;  /* already loaded */

    bp_handle = dlopen(LIBBOOTPOLICY_PATH, RTLD_NOW | RTLD_LOCAL);
    if (!bp_handle) {
        fprintf(stderr, "libbootpolicy: dlopen failed: %s\n", dlerror());
        return -1;
    }

    /* Resolve every symbol we need.  If any is missing we fail
     * immediately so the error message is obvious. */
    RESOLVE(bp_set_log_function,              bootpolicy_set_log_function);
    RESOLVE(bp_error_to_string,               bootpolicy_error_to_string);
    RESOLVE(bp_get_current_os_type,           bootpolicy_get_current_os_type);
    RESOLVE(bp_get_current_os_type_restrictions_override_status,
            bootpolicy_get_current_os_type_restrictions_override_status);
    RESOLVE(bp_has_local_policy_support,      bootpolicy_has_local_policy_support);
    RESOLVE(bp_os_type_to_string,             bootpolicy_os_type_to_string);
    RESOLVE(bp_volume_has_local_policy,       bootpolicy_volume_has_local_policy);
    RESOLVE(bp_get_local_policy,              bootpolicy_get_local_policy);
    RESOLVE(bp_get_local_policy_boolean_tag,  bootpolicy_get_local_policy_boolean_tag);
    RESOLVE(bp_update_local_policy_boolean_tag,
            bootpolicy_update_local_policy_boolean_tag);
    RESOLVE(bp_get_local_policy_integer_tag,  bootpolicy_get_local_policy_integer_tag);
    RESOLVE(bp_get_sip_flags,                bootpolicy_get_sip_flags);
    RESOLVE(bp_update_sip_flags,             bootpolicy_update_sip_flags);
    RESOLVE(bp_remove_sip_flags,             bootpolicy_remove_sip_flags);
    RESOLVE(bp_get_local_policy_pairing_status,
            bootpolicy_get_local_policy_pairing_status);
    RESOLVE(bp_verify_local_policy_pairing,   bootpolicy_verify_local_policy_pairing);
    RESOLVE(bp_update_local_policy_nonce_begin,
            bootpolicy_update_local_policy_nonce_begin);
    RESOLVE(bp_update_local_policy_nonce_end,
            bootpolicy_update_local_policy_nonce_end);
    RESOLVE(bp_update_local_policy_nonce_reset,
            bootpolicy_update_local_policy_nonce_reset);
    RESOLVE(bp_get_security_mode,             bootpolicy_get_security_mode);
    RESOLVE(bp_get_blessed_local_policy_nonce_digest,
            bootpolicy_get_blessed_local_policy_nonce_digest);
    RESOLVE(bp_get_proposed_local_policy_nonce_digest,
            bootpolicy_get_proposed_local_policy_nonce_digest);

    /* Resolve global default value pointers.  These are data symbols
     * (pointers to CFTypeRef), not functions — but dlsym works the same. */
    g_default_volume_path_ptr = (__typeof__(g_default_volume_path_ptr))
        dlsym(bp_handle, "bootpolicy_default_policy_volume_path_value");
    g_default_sfr_path_ptr = (__typeof__(g_default_sfr_path_ptr))
        dlsym(bp_handle, "bootpolicy_default_sfr_manifest_path_value");
    g_root_volume_uuid_ptr = (__typeof__(g_root_volume_uuid_ptr))
        dlsym(bp_handle, "bootpolicy_root_volume_uuid_value");

    if (!g_default_volume_path_ptr)
        fprintf(stderr, "libbootpolicy: warning: default volume path symbol not found\n");
    if (!g_default_sfr_path_ptr)
        fprintf(stderr, "libbootpolicy: warning: default SFR path symbol not found\n");

    return 0;
}

void bp_unload(void)
{
    if (bp_handle) {
        dlclose(bp_handle);
        bp_handle = NULL;
    }
}

bp_cfref_t bp_default_policy_volume_path(void)
{
    return (g_default_volume_path_ptr && *g_default_volume_path_ptr)
        ? *g_default_volume_path_ptr : NULL;
}

bp_cfref_t bp_default_sfr_manifest_path(void)
{
    return (g_default_sfr_path_ptr && *g_default_sfr_path_ptr)
        ? *g_default_sfr_path_ptr : NULL;
}

bp_cfuuid_t bp_root_volume_uuid(void)
{
    return (g_root_volume_uuid_ptr && *g_root_volume_uuid_ptr)
        ? *g_root_volume_uuid_ptr : NULL;
}

bool bp_is_loaded(void)
{
    return bp_handle != NULL;
}

/* ── Forwarding wrappers ──────────────────────────────────────────── */

void bootpolicy_set_log_function(bp_log_fn fn)
{
    if (bp_set_log_function_ptr)
        bp_set_log_function_ptr(fn);
}

const char *bootpolicy_error_to_string(int error)
{
    return bp_error_to_string_ptr
        ? bp_error_to_string_ptr(error)
        : "unknown bootpolicy error";
}

int bootpolicy_get_current_os_type(int *out)
{
    return bp_get_current_os_type_ptr ? bp_get_current_os_type_ptr(out) : -1;
}

int bootpolicy_get_current_os_type_restrictions_override_status(bool *out)
{
    return bp_get_current_os_type_restrictions_override_status_ptr
        ? bp_get_current_os_type_restrictions_override_status_ptr(out) : -1;
}

bool bootpolicy_has_local_policy_support(void)
{
    return bp_has_local_policy_support_ptr
        ? bp_has_local_policy_support_ptr()
        : false;
}

const char *bootpolicy_os_type_to_string(int type)
{
    return bp_os_type_to_string_ptr
        ? bp_os_type_to_string_ptr(type) : "unknown";
}

int bootpolicy_volume_has_local_policy(bp_cfref_t v, bp_cfuuid_t *u, bool *h)
{
    return bp_volume_has_local_policy_ptr
        ? bp_volume_has_local_policy_ptr(v, u, h) : -1;
}

int bootpolicy_get_local_policy(bp_cfref_t v, bp_cfuuid_t u,
                                void **d, size_t *s)
{
    return bp_get_local_policy_ptr
        ? bp_get_local_policy_ptr(v, u, d, s) : -1;
}

int bootpolicy_get_local_policy_boolean_tag(bp_cfref_t v, bp_cfuuid_t u,
                                            const char *t, bool *o)
{
    return bp_get_local_policy_boolean_tag_ptr
        ? bp_get_local_policy_boolean_tag_ptr(v, u, t, o) : -1;
}

int bootpolicy_update_local_policy_boolean_tag(bp_cfref_t v, bp_cfuuid_t u,
                                               bp_cfref_t s, const char *t,
                                               uint32_t id, bool val, void *ctx)
{
    return bp_update_local_policy_boolean_tag_ptr
        ? bp_update_local_policy_boolean_tag_ptr(v, u, s, t, id, val, ctx) : -1;
}

int bootpolicy_get_local_policy_integer_tag(bp_cfref_t v, bp_cfuuid_t u,
                                            const char *t, int64_t *o)
{
    return bp_get_local_policy_integer_tag_ptr
        ? bp_get_local_policy_integer_tag_ptr(v, u, t, o) : -1;
}

int bootpolicy_get_sip_flags(bp_cfref_t v, bp_cfuuid_t u, uint32_t *f)
{
    return bp_get_sip_flags_ptr
        ? bp_get_sip_flags_ptr(v, u, f) : -1;
}

int bootpolicy_update_sip_flags(bp_cfref_t v, bp_cfref_t s,
                                uint32_t f, uint64_t e)
{
    return bp_update_sip_flags_ptr
        ? bp_update_sip_flags_ptr(v, s, f, e) : -1;
}

int bootpolicy_remove_sip_flags(bp_cfref_t v, bp_cfuuid_t u)
{
    return bp_remove_sip_flags_ptr
        ? bp_remove_sip_flags_ptr(v, u) : -1;
}

int bootpolicy_get_local_policy_pairing_status(bp_cfref_t v, bp_cfuuid_t u,
                                               bool s, bool *o)
{
    return bp_get_local_policy_pairing_status_ptr
        ? bp_get_local_policy_pairing_status_ptr(v, u, s, o) : -1;
}

int bootpolicy_verify_local_policy_pairing(bp_cfref_t v, bp_cfref_t r,
                                           bp_cfuuid_t u, bp_cfuuid_t rec,
                                           bool *o)
{
    return bp_verify_local_policy_pairing_ptr
        ? bp_verify_local_policy_pairing_ptr(v, r, u, rec, o) : -1;
}

int bootpolicy_update_local_policy_nonce_begin(void)
{
    return bp_update_local_policy_nonce_begin_ptr
        ? bp_update_local_policy_nonce_begin_ptr() : -1;
}

int bootpolicy_update_local_policy_nonce_end(bp_cfref_t v, bp_cfref_t s,
                                             void *c, bp_cfref_t t)
{
    return bp_update_local_policy_nonce_end_ptr
        ? bp_update_local_policy_nonce_end_ptr(v, s, c, t) : -1;
}

int bootpolicy_update_local_policy_nonce_reset(bp_cfref_t v)
{
    return bp_update_local_policy_nonce_reset_ptr
        ? bp_update_local_policy_nonce_reset_ptr(v) : -1;
}

int bootpolicy_get_security_mode(bp_cfref_t v, int *m)
{
    return bp_get_security_mode_ptr
        ? bp_get_security_mode_ptr(v, m) : -1;
}

int bootpolicy_get_blessed_local_policy_nonce_digest(void *d)
{
    return bp_get_blessed_local_policy_nonce_digest_ptr
        ? bp_get_blessed_local_policy_nonce_digest_ptr(d) : -1;
}

int bootpolicy_get_proposed_local_policy_nonce_digest(void *d)
{
    return bp_get_proposed_local_policy_nonce_digest_ptr
        ? bp_get_proposed_local_policy_nonce_digest_ptr(d) : -1;
}
