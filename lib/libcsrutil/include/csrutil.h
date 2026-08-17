/*
 * csrutil.h — Public API for libcsrutil.
 *
 * This header defines the high-level interface for reading and writing
 * System Integrity Protection (SIP) state.  The implementation lives in
 * csr.c and delegates to libbootpolicy (for LocalPolicy access) and
 * libauthinstall (for ACM credential management).
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_CSRUTIL_H
#define CSRUTIL_CSRUTIL_H

#include <stdint.h>
#include <stdbool.h>

#include "csr.h"

/* ── Error codes ──────────────────────────────────────────────────── */

enum {
    CSRUTIL_OK = 0,
    CSRUTIL_ERR_NOT_ROOT,
    CSRUTIL_ERR_NOT_ARM64,
    CSRUTIL_ERR_NO_POLICY,
    CSRUTIL_ERR_LIB_NOT_LOADED,
    CSRUTIL_ERR_LIB_SYMBOL,
    CSRUTIL_ERR_AUTH_FAILED,
    CSRUTIL_ERR_NONCE_BEGIN,
    CSRUTIL_ERR_NONCE_END,
    CSRUTIL_ERR_WRITE_FAILED,
    CSRUTIL_ERR_READ_FAILED,
    CSRUTIL_ERR_PLATFORM,
    CSRUTIL_ERR_RECOVERY_ONLY,
};

/* ── SIP state snapshot ───────────────────────────────────────────── */

typedef struct {
    uint32_t    csr_config;             /* raw CSR_ALLOW_* bitmask          */

    /* Derived restriction booleans (true = restriction active) */
    bool        kext_restricted;
    bool        fs_restricted;
    bool        debug_restricted;
    bool        dtrace_restricted;
    bool        nvram_restricted;
    bool        kernel_debug;

    /* Convenience flags */
    bool        boot_arg_filter;
    bool        kext_loading;
    bool        apple_internal;
    bool        research_guests;

    /* Security mode (0=Full, 1=Reduced, 2=Permissive) */
    int         security_mode;
    const char *security_mode_name;
} csrutil_state_t;

/* ── Public API ───────────────────────────────────────────────────── */

/* Read the current SIP state into `state`.  Does not require root. */
int  csrutil_status(csrutil_state_t *state);

/* Modify SIP flags.  Both `flags_to_set` and `flags_to_clear` are
 * CSR_ALLOW_* bitmasks.  Authentication is handled internally —
 * if `password` is non-NULL it is used directly; otherwise the user
 * is prompted interactively.
 *
 * Only available on Apple Silicon in Full security mode.  Returns
 * CSRUTIL_OK on success (reboot required for changes to take effect). */
int  csrutil_set_flags(uint32_t flags_to_set,
                       uint32_t flags_to_clear,
                       const char *username,
                       const char *password);

/* Convenience wrappers. */
int  csrutil_disable(const char *username, const char *password);
int  csrutil_enable(const char *username, const char *password);
int  csrutil_reset(const char *username, const char *password);

/* Platform queries. */
bool csrutil_is_apple_silicon(void);
bool csrutil_is_recovery(void);

/* Human-readable error string for a CSRUTIL_ERR_* code. */
const char *csrutil_strerror(int error);

#endif /* CSRUTIL_CSRUTIL_H */
