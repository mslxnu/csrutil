/*
 * main.c — CLI entry point for the open-source csrutil reimplementation.
 *
 * This file provides the command-line interface that dispatches to
 * the libcsrutil API (csrutil_status, csrutil_set_flags, etc.).
 * All SIP logic lives in libcsrutil.a; this file is pure UI.
 *
 * Reverse-engineered from /usr/bin/csrutil on macOS 26 (Tahoe).
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "csrutil.h"

/* ── Version ──────────────────────────────────────────────────────── */

#define CSRUTIL_VERSION "26.0.0"

/* ── Commands ─────────────────────────────────────────────────────── */

typedef enum {
    CMD_NONE = 0,
    CMD_STATUS,
    CMD_ENABLE,
    CMD_DISABLE,
    CMD_CLEAR,
    CMD_GET,
    CMD_SET,
    CMD_WITH,
    CMD_WITHOUT,
    CMD_AUTHENTICATE,
    CMD_VERSION,
    CMD_HELP,
} command_t;

/* ── Flag parsing ─────────────────────────────────────────────────── */

/* Parse a single flag by its csr_flag_table argument name (e.g. "kext")
 * or by its bit-position name (e.g. "csr3").
 * Returns the CSR_ALLOW_* bit value, or -1 on error. */
static int parse_flag(const char *name)
{
    /* Try --without/--with argument style first (e.g. "kext", "fs"). */
    const csr_flag_info_t *info = csr_flag_lookup_by_arg(name);
    if (info) return (int)info->bit;

    /* Try bit-position name (e.g. "csr0" .. "csr13"). */
    if (strncmp(name, "csr", 3) == 0) {
        int idx = atoi(name + 3);
        if (idx < 0 || idx > 13) return -1;
        return (1 << idx);
    }

    /* Try hex literal (e.g. "0x100"). */
    char *end = NULL;
    long val = strtol(name, &end, 0);
    if (end && end != name && *end == '\0' && val > 0 && val <= 0x1FFF)
        return (int)val;

    return -1;
}

/* Parse comma-or-space-separated flag names into a bitmask.
 * Returns the combined bitmask, or -1 on error. */
static uint32_t parse_flags(int argc, char *argv[], int start)
{
    uint32_t flags = 0;
    for (int i = start; i < argc; i++) {
        int flag = parse_flag(argv[i]);
        if (flag < 0) {
            fprintf(stderr, "error: unknown flag '%s'\n", argv[i]);
            return (uint32_t)-1;
        }
        flags |= (uint32_t)flag;
    }
    return flags;
}

/* ── Usage ────────────────────────────────────────────────────────── */

static void print_usage(void)
{
    printf("Usage: csrutil [command] [options]\n"
           "\n"
           "Commands:\n"
           "  status              Display the current SIP status\n"
           "  enable              Enable SIP (set all restrictions)\n"
           "  disable             Disable SIP (clear all restrictions)\n"
           "  clear               Same as enable\n"
           "  get <flag>          Get a specific SIP flag\n"
           "  set <flag> [...]    Set specific SIP flags\n"
           "  --with <arg>        Re-enable a specific restriction\n"
           "  --without <arg>     Disable a specific restriction\n"
           "  authenticate        Test authentication\n"
           "  version             Display version\n"
           "  help                Display this help\n"
           "\n"
           "Flags (by --without argument):\n");

    for (size_t i = 0; i < csr_flag_table_count; i++) {
        const csr_flag_info_t *f = &csr_flag_table[i];
        if (f->csrutil_arg)
            printf("  %-12s 0x%04x  %s\n", f->csrutil_arg, f->bit, f->name);
    }

    printf("\n"
           "Flags (by bit position):\n"
           "  csr0..csr13   equivalent to the CSR_ALLOW_* bits\n"
           "\n"
           "Examples:\n"
           "  csrutil status\n"
           "  csrutil disable\n"
           "  csrutil enable\n"
           "  csrutil --without kext --without fs\n"
           "  csrutil --with kext\n"
           "  csrutil get csr7\n"
           "  csrutil set csr0 csr1\n");
}

/* ── Status display ───────────────────────────────────────────────── */

static void print_status(void)
{
    csrutil_state_t state;
    int rc = csrutil_status(&state);
    if (rc != CSRUTIL_OK) {
        fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
        return;
    }

    printf("System Integrity Protection status: %s\n",
           (state.csr_config == 0) ? "enabled" : "disabled");

    printf("Configuration:\n");
    for (size_t i = 0; i < csr_flag_table_count; i++) {
        const csr_flag_info_t *f = &csr_flag_table[i];
        bool active = (state.csr_config & f->bit) != 0;
        printf("  %-45s %s\n", f->name, active ? "disabled" : "enabled");
    }

    if (state.security_mode_name)
        printf("Security Mode: %s\n", state.security_mode_name);
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    command_t cmd = CMD_NONE;
    int args_start = 1;  /* index of first arg after the command verb */

    /* No getopt — Apple's csrutil treats everything as a positional verb.
     * This avoids clashes between getopt's option parsing and the
     * "--without" / "--with" long-form verbs. */

    if (argc < 2) {
        print_usage();
        return 0;
    }

    /* Parse command verb from argv[1]. */
    const char *verb = argv[1];

    if      (strcmp(verb, "status")       == 0) cmd = CMD_STATUS;
    else if (strcmp(verb, "enable")       == 0) cmd = CMD_ENABLE;
    else if (strcmp(verb, "disable")      == 0) cmd = CMD_DISABLE;
    else if (strcmp(verb, "clear")        == 0) cmd = CMD_CLEAR;
    else if (strcmp(verb, "get")          == 0) cmd = CMD_GET;
    else if (strcmp(verb, "set")          == 0) cmd = CMD_SET;
    else if (strcmp(verb, "--with")       == 0) cmd = CMD_WITH;
    else if (strcmp(verb, "--without")    == 0) cmd = CMD_WITHOUT;
    else if (strcmp(verb, "authenticate") == 0) cmd = CMD_AUTHENTICATE;
    else if (strcmp(verb, "version")      == 0) cmd = CMD_VERSION;
    else if (strcmp(verb, "help")         == 0) cmd = CMD_HELP;
    else if (strcmp(verb, "-h")           == 0) cmd = CMD_HELP;
    else {
        fprintf(stderr, "error: unknown command '%s'\n\n", verb);
        print_usage();
        return 1;
    }
    args_start = 2;

    /* Dispatch. */
    switch (cmd) {

    case CMD_STATUS:
        print_status();
        break;

    case CMD_ENABLE: {
        if (args_start < argc) {
            /* enable --without <flag> --without <flag> ...
             *
             * Collect all --without flags into a bitmask, then enable
             * SIP with those specific allow-bits preserved. */
            uint32_t keep_flags = 0;
            for (int i = args_start; i < argc; i++) {
                bool is_without = (strcmp(argv[i], "--without") == 0);
                bool is_with    = (strcmp(argv[i], "--with") == 0);
                if (is_without || is_with) {
                    if (++i >= argc) {
                        fprintf(stderr, "error: %s requires an argument\n",
                                argv[i - 1]);
                        return 1;
                    }
                    int flag = parse_flag(argv[i]);
                    if (flag < 0) {
                        fprintf(stderr, "error: unknown flag '%s'\n",
                                argv[i]);
                        return 1;
                    }
                    if (is_without)
                        keep_flags |= (uint32_t)flag;
                    /* --with on enable is a no-op (bit already cleared) */
                } else {
                    fprintf(stderr, "error: unexpected argument '%s'\n",
                            argv[i]);
                    return 1;
                }
            }

            /* Set the --without allow-bits, clear everything else. */
            uint32_t to_clear = CSR_VALID_FLAGS & ~keep_flags;
            printf("Enabling System Integrity Protection "
                   "(excluding: 0x%04x).\n", keep_flags);
            int rc = csrutil_set_flags(keep_flags, to_clear, NULL, NULL);
            if (rc != CSRUTIL_OK) {
                fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
                return 1;
            }
        } else {
            int rc = csrutil_enable(NULL, NULL);
            if (rc != CSRUTIL_OK) {
                fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
                return 1;
            }
        }
        break;
    }

    case CMD_DISABLE: {
        if (args_start < argc) {
            /* disable --without <flag> ...
             *
             * Disable SIP but keep specific restrictions active.
             * --without kext on disable means "don't allow untrusted
             * kexts" (keep the kext signing restriction). */
            uint32_t keep_restricted = 0;
            for (int i = args_start; i < argc; i++) {
                bool is_without = (strcmp(argv[i], "--without") == 0);
                bool is_with    = (strcmp(argv[i], "--with") == 0);
                if (is_without || is_with) {
                    if (++i >= argc) {
                        fprintf(stderr, "error: %s requires an argument\n",
                                argv[i - 1]);
                        return 1;
                    }
                    int flag = parse_flag(argv[i]);
                    if (flag < 0) {
                        fprintf(stderr, "error: unknown flag '%s'\n",
                                argv[i]);
                        return 1;
                    }
                    if (is_without)
                        keep_restricted |= (uint32_t)flag;
                } else {
                    fprintf(stderr, "error: unexpected argument '%s'\n",
                            argv[i]);
                    return 1;
                }
            }

            /* Set all allow-bits EXCEPT the ones we want to keep. */
            uint32_t to_set = CSR_SIP_DISABLE_FLAGS & ~keep_restricted;
            printf("Disabling System Integrity Protection "
                   "(keeping restrictions: 0x%04x).\n", keep_restricted);
            int rc = csrutil_set_flags(to_set, 0, NULL, NULL);
            if (rc != CSRUTIL_OK) {
                fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
                return 1;
            }
        } else {
            int rc = csrutil_disable(NULL, NULL);
            if (rc != CSRUTIL_OK) {
                fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
                return 1;
            }
        }
        break;
    }

    case CMD_CLEAR:
        /* Apple's csrutil "clear" == "enable" (restores full SIP). */
        printf("Clearing SIP flags...\n");
        if (csrutil_enable(NULL, NULL) != CSRUTIL_OK) {
            fprintf(stderr, "error: failed to clear SIP flags\n");
            return 1;
        }
        printf("SIP flags cleared.\n");
        break;

    case CMD_GET: {
        if (args_start >= argc) {
            fprintf(stderr, "error: missing flag name\n");
            return 1;
        }
        int flag = parse_flag(argv[args_start]);
        if (flag < 0) {
            fprintf(stderr, "error: unknown flag '%s'\n", argv[args_start]);
            return 1;
        }
        csrutil_state_t state;
        int rc = csrutil_status(&state);
        if (rc != CSRUTIL_OK) {
            fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
            return 1;
        }
        printf("%s: %s\n", argv[args_start],
               (state.csr_config & (uint32_t)flag) ? "disabled" : "enabled");
        break;
    }

    case CMD_SET: {
        if (args_start >= argc) {
            fprintf(stderr, "error: missing flag name(s)\n");
            return 1;
        }
        uint32_t to_set = parse_flags(argc, argv, args_start);
        if (to_set == (uint32_t)-1) return 1;
        printf("Setting SIP flags to 0x%04x...\n", to_set);
        int rc = csrutil_set_flags(to_set, 0, NULL, NULL);
        if (rc != CSRUTIL_OK) {
            fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
            return 1;
        }
        printf("SIP flags set successfully.\n");
        break;
    }

    case CMD_WITH:
    case CMD_WITHOUT: {
        if (args_start >= argc) {
            fprintf(stderr, "error: missing --%s argument\n",
                    cmd == CMD_WITH ? "with" : "without");
            return 1;
        }
        uint32_t flags = parse_flags(argc, argv, args_start);
        if (flags == (uint32_t)-1) return 1;

        if (cmd == CMD_WITHOUT) {
            /* --without means "disable this restriction" → set the allow-bit. */
            printf("Disabling restrictions: 0x%04x\n", flags);
            int rc = csrutil_set_flags(flags, 0, NULL, NULL);
            if (rc != CSRUTIL_OK) {
                fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
                return 1;
            }
        } else {
            /* --with means "re-enable this restriction" → clear the allow-bit. */
            printf("Enabling restrictions: 0x%04x\n", flags);
            int rc = csrutil_set_flags(0, flags, NULL, NULL);
            if (rc != CSRUTIL_OK) {
                fprintf(stderr, "error: %s\n", csrutil_strerror(rc));
                return 1;
            }
        }
        break;
    }

    case CMD_AUTHENTICATE:
        printf("Authenticating...\n");
        /* Trigger an interactive auth to verify credentials. */
        printf("Authentication not yet wired to libcsrutil.\n");
        break;

    case CMD_VERSION:
        printf("csrutil for macOS 26 (Tahoe)\n"
               "Open Source Reimplementation\n"
               "Version: %s\n", CSRUTIL_VERSION);
        break;

    case CMD_HELP:
    default:
        print_usage();
        break;
    }

    return 0;
}
