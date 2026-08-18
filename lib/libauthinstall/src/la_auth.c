/*
 * la_auth.c — LAContext-based authentication (public API).
 *
 * Apple's csrutil authenticates via LocalAuthentication.framework
 * (the PUBLIC API), NOT via the raw ACM C API.  The ACM entitlement
 * (com.apple.private.applecredentialmanager.allow) is only needed
 * by the low-level ACM daemon — LAContext goes through a different
 * code path (TCC/securityd) that doesn't require private entitlements.
 *
 * Flow (derived from Apple's csrutil strings):
 *   1. Create LAContext
 *   2. Set credential (password as NSData, UTF-8) via setCredential:type:
 *   3. Extract externalizedContext (serialized auth proof)
 *   4. Pass to bootpolicy_update_local_policy_nonce_end as acm_token
 *
 * We skip evaluatePolicy:localizedReason:reply: because:
 *   - It's async (dispatches reply block to private queue)
 *   - The Bootability XPC service does its own authentication check
 *   - The externalizedContext is a serialized credential proof, not
 *     a policy evaluation result
 *
 * Credential type values (from LAPublicDefines.h):
 *   kLACredentialTypeApplicationPassword = 0
 *
 * Policy values (from LAPublicDefines.h):
 *   kLAPolicyDeviceOwnerAuthentication  = 2
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include "acm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <termios.h>

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

/* ── Constants (from LAPublicDefines.h) ───────────────────────────── */

#define kLACredentialTypeApplicationPassword  0

/* ── ObjC class references ─────────────────────────────────────────── */

static Class LAContext_class  = NULL;
static Class NSData_class     = NULL;

/* ── Bootstrap ─────────────────────────────────────────────────────── */

static int la_auth_load_class(void)
{
    if (LAContext_class)
        return 0;

    void *handle = dlopen(
        "/System/Library/Frameworks/LocalAuthentication.framework/LocalAuthentication",
        RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "LAAuth: dlopen LocalAuthentication failed: %s\n",
                dlerror());
        return -1;
    }

    LAContext_class = objc_getClass("LAContext");
    NSData_class    = objc_getClass("NSData");

    if (!LAContext_class || !NSData_class) {
        fprintf(stderr, "LAAuth: required classes not found\n");
        return -1;
    }

    return 0;
}

/* ── ObjC message helpers ──────────────────────────────────────────── */

static id msg_send(id self, SEL sel)
{
    return ((id(*)(id, SEL))objc_msgSend)(self, sel);
}

/* ── NSData creation from C string ────────────────────────────────── */

static id nsdata_with_utf8(const char *str)
{
    size_t len = strlen(str);
    return ((id(*)(id, SEL, const void *, unsigned long))objc_msgSend)(
        (id)NSData_class,
        sel_registerName("dataWithBytes:length:"),
        str,
        (unsigned long)len);
}

/* ── LAContext authentication ───────────────────────────────────────── */

/*
 * la_authenticate_password — Set credential on LAContext and extract
 * the externalizedContext.
 *
 * The Bootability XPC service receives this serialized context and
 * performs its own authentication check.  We do NOT call
 * evaluatePolicy: — the credential alone is sufficient for the
 * XPC service to verify we have a valid auth proof.
 *
 * Returns the externalizedContext NSData* as a void* on success.
 * Returns NULL on failure and sets *error_out to a descriptive code.
 */
void *la_authenticate_password(const char *password, int *error_out)
{
    if (error_out) *error_out = 0;

    if (!password || !*password) {
        fprintf(stderr, "LAAuth: no password provided\n");
        if (error_out) *error_out = -1;
        return NULL;
    }

    if (la_auth_load_class() != 0) {
        if (error_out) *error_out = -2;
        return NULL;
    }

    /* Create LAContext: [[LAContext alloc] init] */
    id ctx = msg_send(msg_send((id)LAContext_class, sel_registerName("alloc")),
                      sel_registerName("init"));
    if (!ctx) {
        fprintf(stderr, "LAAuth: failed to create LAContext\n");
        if (error_out) *error_out = -3;
        return NULL;
    }

    /* Convert password to NSData (UTF-8).
     * The credential type for an application-supplied password is 0
     * (kLACredentialTypeApplicationPassword). */
    id ns_password = nsdata_with_utf8(password);

    /* [ctx setCredential:ns_password type:0]
     *
     * setCredential:type: signature:
     *   - (BOOL)setCredential:(NSData *)credential type:(LACredentialType)type
     *
     * LACredentialType is NSInteger (arm64: same size as long). */
    BOOL set_ok = ((BOOL(*)(id, SEL, id, long))objc_msgSend)(
        ctx, sel_registerName("setCredential:type:"),
        ns_password,
        (long)kLACredentialTypeApplicationPassword);

    if (!set_ok) {
        fprintf(stderr, "LAAuth: setCredential failed\n");
        msg_send(ctx, sel_registerName("release"));
        if (error_out) *error_out = -4;
        return NULL;
    }

    /* Extract the externalizedContext.
     * [ctx externalizedContext] — returns NSData*
     *
     * This is the serialized authentication proof that the Bootability
     * XPC service uses to verify the credential was set correctly. */
    id ext_ctx = msg_send(ctx, sel_registerName("externalizedContext"));

    if (!ext_ctx) {
        fprintf(stderr, "LAAuth: externalizedContext is nil\n");
        msg_send(ctx, sel_registerName("release"));
        if (error_out) *error_out = -5;
        return NULL;
    }

    fprintf(stderr, "LAAuth: authentication successful\n");

    /* Retain the externalizedContext so it survives LAContext release. */
    id retained = msg_send(ext_ctx, sel_registerName("retain"));

    /* Release the LAContext — we don't need it anymore. */
    msg_send(ctx, sel_registerName("release"));

    return (void *)retained;
}

/*
 * la_authenticate_interactive — Prompt for password on terminal,
 * then authenticate via LAContext.
 */
void *la_authenticate_interactive(int *error_out)
{
    if (error_out) *error_out = 0;

    fprintf(stderr, "Password: ");
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

    size_t len = strlen(password);
    if (len > 0 && password[len - 1] == '\n')
        password[len - 1] = '\0';

    void *ext_ctx = la_authenticate_password(password, error_out);

    memset(password, 0, sizeof(password));
    return ext_ctx;
}

/*
 * la_release_context — Release an externalizedContext.
 */
void la_release_context(void *ext_ctx)
{
    if (ext_ctx) {
        SEL sel_release = sel_registerName("release");
        ((void(*)(id, SEL))objc_msgSend)((id)ext_ctx, sel_release);
    }
}
