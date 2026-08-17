/*
 * log.h — Logging interface for csrutil.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdbool.h>

/* ── Log levels ───────────────────────────────────────────────────── */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4,
} log_level_t;

/* ── Public API ───────────────────────────────────────────────────── */

/*
 * log_set_level
 *   Sets the minimum log level.
 */
void log_set_level(log_level_t level);

/*
 * log_get_level
 *   Gets the current log level.
 */
log_level_t log_get_level(void);

/*
 * log_set_verbose
 *   Enables or disables verbose output.
 */
void log_set_verbose(bool verbose);

/*
 * log_is_verbose
 *   Returns true if verbose mode is enabled.
 */
bool log_is_verbose(void);

/*
 * log_message
 *   Logs a message at the given level.
 */
void log_message(log_level_t level, const char *fmt, ...);

/* Convenience macros */
#define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_message(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_message(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log_message(LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)

#endif /* LOG_H */
