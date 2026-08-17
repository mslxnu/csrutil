/*
 * bootpolicy.h — Declares the subset of libbootpolicy.dylib we need.
 *
 * libbootpolicy lives inside the dyld shared cache and has no public header.
 * The signatures below were recovered from the arm64e export table and from
 * register-level disassembly of csrutil and bputil.  Types use CoreFoundation
 * aliases so callers never need to link CF directly (opaque pointers suffice).
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_BOOTPOLICY_H
#define CSRUTIL_BOOTPOLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ── Opaque CoreFoundation handles ────────────────────────────────── */

typedef const void *bp_cfref_t;     /* CFTypeRef / CFStringRef / CFUUIDRef */
typedef const void *bp_cfuuid_t;    /* CFUUIDRef (volume UUIDs)           */

/* ── Logging ──────────────────────────────────────────────────────── */

typedef void (*bp_log_fn)(int severity, const char *message);

void bootpolicy_set_log_function(bp_log_fn fn);

/* ── Error handling ───────────────────────────────────────────────── */

const char *bootpolicy_error_to_string(int error);

/* ── OS type ──────────────────────────────────────────────────────── */

int  bootpolicy_get_current_os_type(int *out);
int  bootpolicy_get_current_os_type_restrictions_override_status(bool *out);
bool bootpolicy_has_local_policy_support(void);
const char *bootpolicy_os_type_to_string(int type);

/* ── Volume / policy existence ────────────────────────────────────── */

int bootpolicy_volume_has_local_policy(bp_cfref_t volume_path,
                                       bp_cfuuid_t *uuid_out,
                                       bool *has_local_policy);

int bootpolicy_get_local_policy(bp_cfref_t volume_path,
                                bp_cfuuid_t uuid,
                                void **data_out,
                                size_t *size_out);

/* ── Boolean tags ─────────────────────────────────────────────────── */

int bootpolicy_get_local_policy_boolean_tag(bp_cfref_t volume_path,
                                            bp_cfuuid_t uuid,
                                            const char *tag_name,
                                            bool *value_out);

int bootpolicy_update_local_policy_boolean_tag(bp_cfref_t volume_path,
                                               bp_cfuuid_t uuid,
                                               bp_cfref_t sfr_manifest_path,
                                               const char *tag_name,
                                               uint32_t tag_id,
                                               bool value,
                                               void *acm_context);

/* ── Integer tags ─────────────────────────────────────────────────── */

int bootpolicy_get_local_policy_integer_tag(bp_cfref_t volume_path,
                                            bp_cfuuid_t uuid,
                                            const char *tag_name,
                                            int64_t *value_out);

/* ── SIP flags ────────────────────────────────────────────────────── */

int bootpolicy_get_sip_flags(bp_cfref_t volume_path,
                             bp_cfuuid_t uuid,
                             uint32_t *flags_out);

int bootpolicy_update_sip_flags(bp_cfref_t volume_path,
                                bp_cfref_t sfr_manifest_path,
                                uint32_t flags,
                                uint64_t flags_mask);

int bootpolicy_remove_sip_flags(bp_cfref_t volume_path,
                                bp_cfuuid_t uuid);

/* ── Pairing ──────────────────────────────────────────────────────── */

int bootpolicy_get_local_policy_pairing_status(bp_cfref_t volume_path,
                                               bp_cfuuid_t uuid,
                                               bool strict,
                                               bool *paired_out);

int bootpolicy_verify_local_policy_pairing(bp_cfref_t volume_path,
                                           bp_cfref_t recovery_path,
                                           bp_cfuuid_t uuid,
                                           bp_cfuuid_t recovery_uuid,
                                           bool *paired_out);

/* ── Nonce lifecycle ──────────────────────────────────────────────── */
/*
 * Every modification to the LocalPolicy must be bracketed by a
 * nonce rotation.  The SEP consumes the nonce as a freshness
 * guarantee and signs the resulting policy blob.
 *
 * Sequence:
 *   nonce_begin()
 *   modify tags and/or sip flags
 *   nonce_end(volume, sfr_path, acm_context, acm_token)
 *
 * If any step fails, call nonce_reset() to abort the transaction.
 */

int bootpolicy_update_local_policy_nonce_begin(void);

int bootpolicy_update_local_policy_nonce_end(bp_cfref_t volume_path,
                                             bp_cfref_t sfr_manifest_path,
                                             void *acm_context,
                                             bp_cfref_t acm_token);

int bootpolicy_update_local_policy_nonce_reset(bp_cfref_t volume_path);

/* ── Security mode ────────────────────────────────────────────────── */

int bootpolicy_get_security_mode(bp_cfref_t volume_path, int *mode_out);

/* ── Nonce digests (diagnostic) ───────────────────────────────────── */

int bootpolicy_get_blessed_local_policy_nonce_digest(void *digest_out);
int bootpolicy_get_proposed_local_policy_nonce_digest(void *digest_out);

/* ── Bootstrap (internal) ────────────────────────────────────────── */

/* dlopen libbootpolicy and resolve all symbols.  Returns 0 on success.
 * Safe to call multiple times (no-op after the first successful load). */
int  bp_load(void);

/* Release the dlopen handle. */
void bp_unload(void);

/* Is libbootpolicy loaded and ready? */
bool bp_is_loaded(void);

/* ── Global default value accessors (resolved via dlsym) ───────────── */

bp_cfref_t  bp_default_policy_volume_path(void);
bp_cfref_t  bp_default_sfr_manifest_path(void);
bp_cfuuid_t bp_root_volume_uuid(void);

#endif /* CSRUTIL_BOOTPOLICY_H */
