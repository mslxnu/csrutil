/*
 * log.c — Logging implementation.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

/* ── Global state ──────────────────────────────────────────────────── */

static log_level_t current_level = LOG_LEVEL_WARN;
static bool verbose_mode = false;

/* ── Level names ───────────────────────────────────────────────────── */

static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
    NULL
};

/* ── Public API ───────────────────────────────────────────────────── */

void log_set_level(log_level_t level) {
    current_level = level;
}

log_level_t log_get_level(void) {
    return current_level;
}

void log_set_verbose(bool verbose) {
    verbose_mode = verbose;
    if (verbose) {
        current_level = LOG_LEVEL_DEBUG;
    }
}

bool log_is_verbose(void) {
    return verbose_mode;
}

void log_message(log_level_t level, const char *fmt, ...) {
    if (level < current_level) {
        return;
    }

    /* Get timestamp */
    struct timeval tv;
    struct tm tm_info;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_info);

    /* Print timestamp and level */
    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d.%03d [%s] ",
            tm_info.tm_year + 1900,
            tm_info.tm_mon + 1,
            tm_info.tm_mday,
            tm_info.tm_hour,
            tm_info.tm_min,
            tm_info.tm_sec,
            (int)(tv.tv_usec / 1000),
            level_names[level]);

    /* Print message */
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}
