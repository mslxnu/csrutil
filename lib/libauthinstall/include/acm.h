/*
 * acm.h — Apple Credential Manager (ACM) client API.
 *
 * The ACM layer sits between userspace tools (csrutil, bputil) and the
 * Secure Enclave.  It provides a credential-verified context that the
 * SEP trusts for signing policy mutations.
 *
 * On macOS 26 (Tahoe), the ACM C API lives inside
 * LocalAuthenticationCore.framework.  The old ACMInitialize / ACMTerminate
 * symbols have been removed — the subsystem is ready to use immediately
 * after dlopen().
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

/* ── Credential types (from the strings in LocalAuthenticationCore) ── */

enum acm_credential_type {
    ACM_CREDENTIAL_TYPE_PASSCODE_VALIDATED = 1,
    /* 2 = biometric (Touch ID / Face ID) */
    /* 3 = token (smart card)             */
};

/* ── Policy names (from the csrutil disassembly) ──────────────────── */

#define ACM_AUTH_POLICY_LOCAL_USER   "authenticate_local_user"
#define ACM_AUTH_POLICY_RECOVERY     "authenticate_recovery_user_for_install"

/* ── Bootstrap ────────────────────────────────────────────────────── */

/* dlopen LocalAuthenticationCore.framework and resolve all symbols.
 * Returns 0 on success.  Safe to call multiple times. */
int  acm_load(void);

/* Returns true after a successful acm_load(). */
bool acm_is_loaded(void);

/* ── Context lifecycle ────────────────────────────────────────────── */

acm_context_t acm_context_create(uint32_t flags);
void          acm_context_delete(acm_context_t ctx);

/* ── Credential management ────────────────────────────────────────── */

int  acm_context_add_credential(acm_context_t ctx, acm_credential_t cred);
int  acm_context_remove_credentials_by_type(acm_context_t ctx, uint32_t type);
bool acm_context_contains_credential_type(acm_context_t ctx, uint32_t type);

/* ── Credential creation (new API on macOS 26) ────────────────────── */

acm_credential_t acm_credential_create(uint32_t type);
void             acm_credential_delete(acm_credential_t cred);
int  acm_credential_set_property(acm_credential_t cred,
                                 uint32_t property,
                                 const void *data, size_t len);

/* ── Policy verification ──────────────────────────────────────────── */

int  acm_context_verify_policy(acm_context_t      ctx,
                               const char        *policy_name,
                               acm_requirement_t *out_req);

int  acm_context_verify_policy_ex(acm_context_t      ctx,
                                  const char        *policy_name,
                                  uint32_t           flags,
                                  acm_requirement_t *out_req);

void acm_requirement_delete(acm_requirement_t req);
int  acm_requirement_get_state(acm_requirement_t req, uint32_t *out_state);

/* ── Convenience: password authentication ─────────────────────────── */

/* Authenticate using a username + password pair.  Returns a context
 * that has been verified against the "authenticate_local_user" policy
 * and is trusted by the SEP for signing policy mutations.
 * The caller must acm_context_delete() the returned context. */
acm_context_t acm_authenticate_with_password(const char *username,
                                             const char *password,
                                             int        *error_out);

/* Variant that prompts interactively via terminal echo. */
acm_context_t acm_authenticate_interactive(const char *username,
                                           int        *error_out);

#endif /* CSRUTIL_ACM_H */
