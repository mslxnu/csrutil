/*
 * acm.c — Dynamic loader for libauthinstall.dylib (ACM symbols).
 *
 * The Apple Credential Manager (ACM) functions live inside
 * /usr/lib/libauthinstall.dylib which is in the dyld shared cache.
 * We dlopen() it and resolve every symbol via dlsym(), following
 * the same pattern as bootpolicy.c for libbootpolicy.dylib.
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

#define LIBAUTHINSTALL_PATH "/usr/lib/libauthinstall.dylib"

/* ── Function pointer table ───────────────────────────────────────── */

#define ACM_FUNC(ret, name, ...) \
    typedef ret (*name##_fn)(__VA_ARGS__); \
    static name##_fn name##_ptr = NULL;

ACM_FUNC(int, acm_Initialize, void)
ACM_FUNC(void, acm_Terminate, void)

ACM_FUNC(acm_context_t, acm_ContextCreate, uint32_t flags)
ACM_FUNC(void, acm_ContextDelete, acm_context_t ctx)

ACM_FUNC(int, acm_ContextAddCredential, acm_context_t ctx, acm_credential_t cred)
ACM_FUNC(int, acm_ContextRemoveCredentialsByType, acm_context_t ctx, uint32_t type)
ACM_FUNC(int, acm_ContextContainsCredentialType, acm_context_t ctx, uint32_t type)

ACM_FUNC(int, acm_ContextVerifyPolicy, acm_context_t ctx,
         const char *policy_name, acm_requirement_t *out_req)
ACM_FUNC(int, acm_ContextVerifyPolicyEx, acm_context_t ctx,
         const char *policy_name, uint32_t flags, acm_requirement_t *out_req)

ACM_FUNC(void, acm_RequirementDelete, acm_requirement_t req)

ACM_FUNC(acm_credential_t, acm_CredentialCreatePassword,
         const char *username, const char *password)

ACM_FUNC(int, acm_AuthenticateWithPassword, acm_context_t ctx,
         const char *username, const char *password)
ACM_FUNC(int, acm_AuthenticateInteractive, acm_context_t ctx,
         const char *username)

#undef ACM_FUNC

/* ── Symbol resolution ────────────────────────────────────────────── */

#define RESOLVE(var, name) \
    do { \
        var##_ptr = (__typeof__(var##_ptr))dlsym(acm_handle, #name); \
        /* Not all symbols are mandatory — log warnings for optional ones */ \
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

    acm_handle = dlopen(LIBAUTHINSTALL_PATH, RTLD_NOW | RTLD_LOCAL);
    if (!acm_handle) {
        fprintf(stderr, "libauthinstall: dlopen failed: %s\n", dlerror());
        return -1;
    }

    RESOLVE_REQUIRED(acm_Initialize, ACMInitialize);
    RESOLVE(acm_Terminate, ACMTerminate);
    RESOLVE_REQUIRED(acm_ContextCreate, ACMContextCreate);
    RESOLVE_REQUIRED(acm_ContextDelete, ACMContextDelete);
    RESOLVE(acm_ContextAddCredential, ACMContextAddCredential);
    RESOLVE(acm_ContextRemoveCredentialsByType, ACMContextRemoveCredentialsByType);
    RESOLVE(acm_ContextContainsCredentialType, ACMContextContainsCredentialType);
    RESOLVE_REQUIRED(acm_ContextVerifyPolicy, ACMContextVerifyPolicy);
    RESOLVE(acm_ContextVerifyPolicyEx, ACMContextVerifyPolicyEx);
    RESOLVE(acm_RequirementDelete, ACMRequirementDelete);
    RESOLVE(acm_CredentialCreatePassword, ACMCredentialCreatePassword);
    RESOLVE_REQUIRED(acm_AuthenticateWithPassword, ACMAuthenticateWithPassword);
    RESOLVE_REQUIRED(acm_AuthenticateInteractive, ACMAuthenticateInteractive);

    /* Initialize the ACM subsystem. */
    int rc = acm_Initialize_ptr();
    if (rc != 0) {
        fprintf(stderr, "libauthinstall: ACMInitialize failed: %d\n", rc);
        dlclose(acm_handle);
        acm_handle = NULL;
        return -1;
    }

    return 0;
}

/* ── Context lifecycle ────────────────────────────────────────────── */

acm_context_t acm_context_create(uint32_t flags)
{
    if (!acm_handle || !acm_ContextCreate_ptr)
        return NULL;
    return acm_ContextCreate_ptr(flags);
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

    /* Authenticate with the ACM backend. */
    if (acm_AuthenticateWithPassword_ptr) {
        int rc = acm_AuthenticateWithPassword_ptr(ctx, username, password);
        if (rc != 0) {
            if (error_out) *error_out = rc;
            acm_context_delete(ctx);
            return NULL;
        }
    }

    return ctx;
}

acm_context_t acm_authenticate_interactive(const char *username,
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

    /* Use the interactive ACM authentication path. */
    if (acm_AuthenticateInteractive_ptr) {
        int rc = acm_AuthenticateInteractive_ptr(ctx, username);
        if (rc != 0) {
            if (error_out) *error_out = rc;
            acm_context_delete(ctx);
            return NULL;
        }
    } else {
        /* Fallback: prompt for password on terminal. */
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

        acm_context_delete(ctx);
        ctx = acm_authenticate_with_password(username, password, error_out);

        memset(password, 0, sizeof(password));
    }

    return ctx;
}
