/*
 * csr.h — System Integrity Protection flag definitions.
 *
 * Mirrored from XNU bsd/sys/csr.h (APSL-licensed).  The bit layout is the
 * authoritative contract between boot.efi / iBoot, the kernel, and every
 * userspace tool that reads or writes SIP state (csrutil, bputil, csrstat,
 * and now this tool).
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_CSR_H
#define CSRUTIL_CSR_H

#include <stddef.h>
#include <stdint.h>

/* ── Individual flag bits ─────────────────────────────────────────── */

#define CSR_ALLOW_UNTRUSTED_KEXTS               (1U <<  0)  /* 0x0001 */
#define CSR_ALLOW_UNRESTRICTED_FS               (1U <<  1)  /* 0x0002 */
#define CSR_ALLOW_TASK_FOR_PID                  (1U <<  2)  /* 0x0004 */
#define CSR_ALLOW_KERNEL_DEBUGGER               (1U <<  3)  /* 0x0008 */
#define CSR_ALLOW_APPLE_INTERNAL                (1U <<  4)  /* 0x0010 */
#define CSR_ALLOW_UNRESTRICTED_DTRACE           (1U <<  5)  /* 0x0020 */
#define CSR_ALLOW_UNRESTRICTED_NVRAM            (1U <<  6)  /* 0x0040 */
#define CSR_ALLOW_DEVICE_CONFIGURATION          (1U <<  7)  /* 0x0080 */
#define CSR_ALLOW_ANY_RECOVERY_OS               (1U <<  8)  /* 0x0100 */
#define CSR_ALLOW_UNAPPROVED_KEXTS              (1U <<  9)  /* 0x0200 */
#define CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE    (1U << 10)  /* 0x0400 */
#define CSR_ALLOW_AUTHENTICATED_ROOT_REQUIREMENT (1U << 11) /* 0x0800 */
#define CSR_ALLOW_RESEARCH_GUESTS               (1U << 12)  /* 0x1000 */

/* ── Valid flags mask ─────────────────────────────────────────────── */

#define CSR_VALID_FLAGS \
    (CSR_ALLOW_UNTRUSTED_KEXTS          | \
     CSR_ALLOW_UNRESTRICTED_FS          | \
     CSR_ALLOW_TASK_FOR_PID             | \
     CSR_ALLOW_KERNEL_DEBUGGER          | \
     CSR_ALLOW_APPLE_INTERNAL           | \
     CSR_ALLOW_UNRESTRICTED_DTRACE      | \
     CSR_ALLOW_UNRESTRICTED_NVRAM       | \
     CSR_ALLOW_DEVICE_CONFIGURATION     | \
     CSR_ALLOW_ANY_RECOVERY_OS          | \
     CSR_ALLOW_UNAPPROVED_KEXTS         | \
     CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE | \
     CSR_ALLOW_AUTHENTICATED_ROOT_REQUIREMENT | \
     CSR_ALLOW_RESEARCH_GUESTS)

/* ── Flags for enable / disable convenience ──────────────────────── */

#define CSR_SIP_DISABLE_FLAGS \
    (CSR_ALLOW_UNTRUSTED_KEXTS          | \
     CSR_ALLOW_UNRESTRICTED_FS          | \
     CSR_ALLOW_TASK_FOR_PID             | \
     CSR_ALLOW_KERNEL_DEBUGGER          | \
     CSR_ALLOW_UNRESTRICTED_DTRACE      | \
     CSR_ALLOW_UNRESTRICTED_NVRAM       | \
     CSR_ALLOW_DEVICE_CONFIGURATION     | \
     CSR_ALLOW_ANY_RECOVERY_OS          | \
     CSR_ALLOW_UNAPPROVED_KEXTS         | \
     CSR_ALLOW_RESEARCH_GUESTS)

#define CSR_SIP_ENABLE_FLAGS   0U   /* clear all allow-bits = full SIP */

/* ── Flag metadata ────────────────────────────────────────────────── */

typedef struct {
    uint32_t    bit;
    const char *name;            /* human-readable, e.g. "Kext Signing"         */
    const char *description;     /* one-liner                                   */
    const char *csrutil_arg;     /* e.g. "--without kext", or NULL if hidden    */
} csr_flag_info_t;

/* Populated at compile time — no allocation. */
extern const csr_flag_info_t csr_flag_table[];
extern const size_t          csr_flag_table_count;

/* Look up a flag by its `--without` argument string (e.g. "kext").
 * Returns NULL when the argument is not recognised. */
const csr_flag_info_t *csr_flag_lookup_by_arg(const char *arg);

/* Look up a flag by bit position.  Returns NULL for unknown bits. */
const csr_flag_info_t *csr_flag_lookup_by_bit(uint32_t bit);

#endif /* CSRUTIL_CSR_H */
