/*
 * la_auth.h — LAContext-based authentication (public API).
 *
 * This module provides authentication via the public
 * LocalAuthentication.framework, bypassing the raw ACM C API
 * that requires private entitlements.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_LA_AUTH_H
#define CSRUTIL_LA_AUTH_H

#include <stdbool.h>

/*
 * la_authenticate_password — Authenticate with a password.
 * Returns a retained externalizedContext (NSData*) as void*.
 * Caller must la_release_context() when done.
 */
void *la_authenticate_password(const char *password, int *error_out);

/*
 * la_authenticate_interactive — Prompt on terminal, then authenticate.
 * Returns a retained externalizedContext (NSData*) as void*.
 * Caller must la_release_context() when done.
 */
void *la_authenticate_interactive(int *error_out);

/*
 * la_release_context — Release an externalizedContext.
 */
void la_release_context(void *ext_ctx);

#endif /* CSRUTIL_LA_AUTH_H */
