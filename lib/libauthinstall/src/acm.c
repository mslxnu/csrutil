/*
 * acm.c — Dynamic loader for ACM (Apple Credential Manager) symbols.
 *
 * On macOS 26 (Tahoe), the ACM C API lives inside
 * LocalAuthenticationCore.framework rather than libauthinstall.dylib.
 * The old ACMInitialize / ACMTerminate symbols have been removed —
 * the subsystem no longer requires explicit initialization.
 *
 * We dlopen() the framework and resolve every symbol we need via
 * dlsym(), following the same pattern as bootpolicy.c.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include "acm.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/* ── dylib handle ─────────────────────────────────────────────────── */

static void *acm_handle = NULL;

/* On macOS 26, ACM symbols moved out of libauthinstall.dylib and into
 * LocalAuthenticationCore.framework.  dyld resolves the framework path
 * from the shared cache — the file on disk doesn't need to exist as a
 * standalone .dylib. */
#define LIBACM_PATH "/System/Library/PrivateFrameworks/LocalAuthenticationCore.framework/LocalAuthenticationCore"

/* ── Function pointer table ───────────────────────────────────────── */

#define ACM_FUNC(ret, name, ...) \
    typedef ret (*name##_fn)(__VA_ARGS__); \
    static name##_fn name##_ptr = NULL;

/* Context lifecycle — returns int error code, writes context to *out */
ACM_FUNC(int, acm_ContextCreate, uint32_t flags, acm_context_t *out)
ACM_FUNC(void, acm_ContextDelete, acm_context_t ctx)

/* Credential management */
ACM_FUNC(int, acm_ContextAddCredential, acm_context_t ctx, acm_credential_t cred)
ACM_FUNC(int, acm_ContextRemoveCredentialsByType, acm_context_t ctx, uint32_t type)
ACM_FUNC(int, acm_ContextContainsCredentialType, acm_context_t ctx, uint32_t type)

/* Policy verification */
ACM_FUNC(int, acm_ContextVerifyPolicy, acm_context_t ctx,
         const char *policy_name, acm_requirement_t *out_req)
ACM_FUNC(int, acm_ContextVerifyPolicyEx, acm_context_t ctx,
         const char *policy_name, uint32_t flags, acm_requirement_t *out_req)

/* Requirement inspection */
ACM_FUNC(void, acm_RequirementDelete, acm_requirement_t req)
ACM_FUNC(int, acm_RequirementGetState, acm_requirement_t req, uint32_t *out_state)
ACM_FUNC(int, acm_RequirementGetType, acm_requirement_t req, uint32_t *out_type)

/* Credential creation / properties (new API on macOS 26) */
ACM_FUNC(int, acm_CredentialCreate, uint32_t type, acm_credential_t *out)
ACM_FUNC(void, acm_CredentialDelete, acm_credential_t cred)
ACM_FUNC(int, acm_CredentialSetProperty, acm_credential_t cred,
         uint32_t property, const void *data, size_t len)
ACM_FUNC(int, acm_CredentialGetPropertyData, acm_credential_t cred,
         uint32_t property, size_t *inout_len, void *out)

#undef ACM_FUNC

/* ── Symbol resolution ────────────────────────────────────────────── */

#define RESOLVE(var, name) \
    do { \
        var##_ptr = (__typeof__(var##_ptr))dlsym(acm_handle, #name); \
    } while (0)

#define RESOLVE_REQUIRED(var, name) \
    do { \
        var##_ptr = (__typeof__(var##_ptr))dlsym(acm_handle, #name); \
        if (!var##_ptr) { \
            fprintf(stderr, "libauthinstall: missing required symbol: %s\n", #name); \
            return -1; \
        } \
    } while (0)

/* ── Bootstrap ────────────────────────────────────────────────────── */

int acm_load(void)
{
    if (acm_handle)
        return 0;

    acm_handle = dlopen(LIBACM_PATH, RTLD_NOW | RTLD_LOCAL);
    if (!acm_handle) {
        fprintf(stderr, "libauthinstall: dlopen failed: %s\n", dlerror());
        return -1;
    }

    /* Context lifecycle — mandatory. */
    RESOLVE_REQUIRED(acm_ContextCreate, ACMContextCreate);
    RESOLVE_REQUIRED(acm_ContextDelete, ACMContextDelete);

    /* Credential management — mandatory. */
    RESOLVE_REQUIRED(acm_ContextAddCredential, ACMContextAddCredential);
    RESOLVE(acm_ContextRemoveCredentialsByType, ACMContextRemoveCredentialsByType);
    RESOLVE(acm_ContextContainsCredentialType, ACMContextContainsCredentialType);

    /* Policy verification — mandatory. */
    RESOLVE_REQUIRED(acm_ContextVerifyPolicy, ACMContextVerifyPolicy);
    RESOLVE(acm_ContextVerifyPolicyEx, ACMContextVerifyPolicyEx);

    /* Requirement inspection. */
    RESOLVE(acm_RequirementDelete, ACMRequirementDelete);
    RESOLVE(acm_RequirementGetState, ACMRequirementGetState);
    RESOLVE(acm_RequirementGetType, ACMRequirementGetType);

    /* Credential creation — new API. */
    RESOLVE(acm_CredentialCreate, ACMCredentialCreate);
    RESOLVE(acm_CredentialDelete, ACMCredentialDelete);
    RESOLVE(acm_CredentialSetProperty, ACMCredentialSetProperty);
    RESOLVE(acm_CredentialGetPropertyData, ACMCredentialGetPropertyData);

    /* No initialization needed on macOS 26 — the old ACMInitialize
     * symbol no longer exists.  The subsystem is ready to use
     * immediately after dlopen(). */

    return 0;
}

bool acm_is_loaded(void)
{
    return acm_handle != NULL;
}

/* ── Context lifecycle ────────────────────────────────────────────── */

acm_context_t acm_context_create(uint32_t flags)
{
    if (!acm_handle || !acm_ContextCreate_ptr)
        return NULL;
    acm_context_t ctx = NULL;
    int rc = acm_ContextCreate_ptr(flags, &ctx);
    if (rc != 0) {
        fprintf(stderr, "ACM: ACMContextCreate failed: %d\n", rc);
        if (rc == -3) {
            fprintf(stderr, "ACM: The ACM subsystem requires the "
                    "com.apple.private.applecredentialmanager.allow\n"
                    "ACM: entitlement, which can only be granted to "
                    "Apple-signed binaries.\n"
                    "ACM: csrutil cannot acquire this entitlement on "
                    "macOS 26.\n"
                    "ACM: Use Apple's /usr/bin/csrutil for SIP "
                    "modifications, or run from Recovery OS.\n");
        }
        return NULL;
    }
    return ctx;
}

void acm_context_delete(acm_context_t ctx)
{
    if (ctx && acm_ContextDelete_ptr)
        acm_ContextDelete_ptr(ctx);
}

/* ── Credential management ────────────────────────────────────────── */

int acm_context_add_credential(acm_context_t ctx, acm_credential_t cred)
{
    if (!acm_ContextAddCredential_ptr)
        return -1;
    return acm_ContextAddCredential_ptr(ctx, cred);
}

int acm_context_remove_credentials_by_type(acm_context_t ctx, uint32_t type)
{
    if (!acm_ContextRemoveCredentialsByType_ptr)
        return -1;
    return acm_ContextRemoveCredentialsByType_ptr(ctx, type);
}

bool acm_context_contains_credential_type(acm_context_t ctx, uint32_t type)
{
    if (!acm_ContextContainsCredentialType_ptr)
        return false;
    return acm_ContextContainsCredentialType_ptr(ctx, type) != 0;
}

/* ── Credential creation (new API) ────────────────────────────────── */

acm_credential_t acm_credential_create(uint32_t type)
{
    if (!acm_CredentialCreate_ptr)
        return NULL;
    acm_credential_t cred = NULL;
    int rc = acm_CredentialCreate_ptr(type, &cred);
    return (rc == 0) ? cred : NULL;
}

void acm_credential_delete(acm_credential_t cred)
{
    if (cred && acm_CredentialDelete_ptr)
        acm_CredentialDelete_ptr(cred);
}

int acm_credential_set_property(acm_credential_t cred,
                                uint32_t property,
                                const void *data, size_t len)
{
    if (!acm_CredentialSetProperty_ptr)
        return -1;
    return acm_CredentialSetProperty_ptr(cred, property, data, len);
}

/* ── Policy verification ──────────────────────────────────────────── */

int acm_context_verify_policy(acm_context_t ctx,
                              const char *policy_name,
                              acm_requirement_t *out_req)
{
    if (!acm_ContextVerifyPolicy_ptr)
        return -1;
    return acm_ContextVerifyPolicy_ptr(ctx, policy_name, out_req);
}

int acm_context_verify_policy_ex(acm_context_t ctx,
                                 const char *policy_name,
                                 uint32_t flags,
                                 acm_requirement_t *out_req)
{
    if (!acm_ContextVerifyPolicyEx_ptr)
        return -1;
    return acm_ContextVerifyPolicyEx_ptr(ctx, policy_name, flags, out_req);
}

void acm_requirement_delete(acm_requirement_t req)
{
    if (req && acm_RequirementDelete_ptr)
        acm_RequirementDelete_ptr(req);
}

int acm_requirement_get_state(acm_requirement_t req, uint32_t *out_state)
{
    if (!acm_RequirementGetState_ptr || !out_state)
        return -1;
    return acm_RequirementGetState_ptr(req, out_state);
}

/* ── Convenience: password authentication ─────────────────────────── */

acm_context_t acm_authenticate_with_password(const char *username,
                                             const char *password,
                                             int *error_out)
{
    if (error_out) *error_out = 0;

    if (!acm_handle) {
        if (error_out) *error_out = -1;
        return NULL;
    }

    /* Create a context. */
    acm_context_t ctx = acm_context_create(0);
    if (!ctx) {
        if (error_out) *error_out = -2;
        return NULL;
    }

    /* Create a passcode-validated credential (type 1). */
    if (acm_CredentialCreate_ptr) {
        acm_credential_t cred = NULL;
        int rc = acm_CredentialCreate_ptr(
            ACM_CREDENTIAL_TYPE_PASSCODE_VALIDATED, &cred);
        if (rc != 0 || !cred) {
            if (error_out) *error_out = rc ? rc : -3;
            acm_context_delete(ctx);
            return NULL;
        }

        /* Set the password on the credential.  Property 0x10000 is
         * the passcode data in the ACM credential property space. */
        if (password && acm_CredentialSetProperty_ptr) {
            acm_CredentialSetProperty_ptr(cred, 0x10000,
                password, strlen(password));
        }

        /* Set the username if provided.  Property 0x10001 is the
         * user identifier. */
        if (username && acm_CredentialSetProperty_ptr) {
            acm_CredentialSetProperty_ptr(cred, 0x10001,
                username, strlen(username));
        }

        /* Add the credential to the context. */
        rc = acm_context_add_credential(ctx, cred);

        /* The credential handle is copied into the context — we can
         * release our reference now. */
        if (acm_CredentialDelete_ptr)
            acm_CredentialDelete_ptr(cred);

        if (rc != 0) {
            if (error_out) *error_out = rc;
            acm_context_delete(ctx);
            return NULL;
        }
    }

    /* Verify the "authenticate local user" policy.  If this succeeds,
     * the context is authenticated and trusted by the SEP. */
    acm_requirement_t req = NULL;
    int rc = acm_context_verify_policy(ctx, ACM_AUTH_POLICY_LOCAL_USER, &req);

    if (rc != 0) {
        /* Verification failed — context is not authenticated. */
        if (error_out) *error_out = rc;
        acm_requirement_delete(req);
        acm_context_delete(ctx);
        return NULL;
    }

    /* Check the requirement state.  State 0 = requirement satisfied. */
    if (acm_RequirementGetState_ptr && req) {
        uint32_t state = (uint32_t)-1;
        acm_RequirementGetState_ptr(req, &state);
        if (state != 0) {
            if (error_out) *error_out = (int)state;
            acm_requirement_delete(req);
            acm_context_delete(ctx);
            return NULL;
        }
    }

    acm_requirement_delete(req);
    return ctx;
}

acm_context_t acm_authenticate_interactive(const char *username,
                                           int *error_out)
{
    if (error_out) *error_out = 0;

    /* Prompt for password on terminal, then delegate. */
    fprintf(stderr, "Password for %s: ", username ? username : "root");
    fflush(stderr);

    struct termios old, new_term;
    tcgetattr(STDIN_FILENO, &old);
    new_term = old;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    char password[256] = {0};
    fgets(password, sizeof(password), stdin);

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    fprintf(stderr, "\n");

    /* Strip trailing newline. */
    size_t len = strlen(password);
    if (len > 0 && password[len - 1] == '\n')
        password[len - 1] = '\0';

    acm_context_t ctx = acm_authenticate_with_password(username, password,
                                                       error_out);

    memset(password, 0, sizeof(password));
    return ctx;
}
