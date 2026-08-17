/*
 * acm.h — Apple Credential Manager (ACM) client API.
 *
 * The ACM layer sits between userspace tools (csrutil, bputil) and the
 * Secure Enclave.  It provides a credential-verified context that the
 * SEP trusts for signing policy mutations.
 *
 * These declarations come from reverse-engineering the acmlib exports
 * inside libauthinstall.dylib and from the csrutil / bputil disassembly.
 * No public header exists.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_ACM_H
#define CSRUTIL_ACM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ── Opaque handle ────────────────────────────────────────────────── */

typedef void *acm_context_t;
typedef void *acm_requirement_t;
typedef void *acm_credential_t;

/* ── Credential types (from the strings in libauthinstall) ────────── */

enum acm_credential_type {
    ACM_CREDENTIAL_TYPE_PASSCODE_VALIDATED = 1,
    /* 2 = biometric (Touch ID / Face ID) */
    /* 3 = token (smart card)             */
};

/* ── Policy names (from the csrutil disassembly) ──────────────────── */

#define ACM_AUTH_POLICY_LOCAL_USER   "authenticate_local_user"
#define ACM_AUTH_POLICY_RECOVERY     "authenticate_recovery_user_for_install"

/* ── Bootstrap ────────────────────────────────────────────────────── */

/* dlopen libauthinstall and resolve all symbols.  Returns 0 on success.
 * Safe to call multiple times (no-op after the first successful load). */
int acm_load(void);

/* ── Context lifecycle ────────────────────────────────────────────── */

acm_context_t acm_context_create(uint32_t flags);
void          acm_context_delete(acm_context_t ctx);

/* ── Credential management ────────────────────────────────────────── */

int  acm_context_add_credential(acm_context_t ctx, acm_credential_t cred);
int  acm_context_remove_credentials_by_type(acm_context_t ctx, uint32_t type);
bool acm_context_contains_credential_type(acm_context_t ctx, uint32_t type);

/* ── Policy verification ──────────────────────────────────────────── */

int  acm_context_verify_policy(acm_context_t      ctx,
                               const char        *policy_name,
                               acm_requirement_t *out_req);

int  acm_context_verify_policy_ex(acm_context_t      ctx,
                                  const char        *policy_name,
                                  uint32_t           flags,
                                  acm_requirement_t *out_req);

void acm_requirement_delete(acm_requirement_t req);

/* ── Convenience: password authentication ─────────────────────────── */

/* Authenticate using a username + password pair.  Returns a context
 * that can be passed to bootpolicy_update_local_policy_nonce_end().
 * The caller must acm_context_delete() the returned context. */
acm_context_t acm_authenticate_with_password(const char *username,
                                             const char *password,
                                             int        *error_out);

/* Variant that prompts interactively via terminal echo. */
acm_context_t acm_authenticate_interactive(const char *username,
                                           int        *error_out);

#endif /* CSRUTIL_ACM_H */
