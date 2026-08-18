# CLAUDE.md — csrutil project context

## What This Is
Open-source reimplementation of `/usr/bin/csrutil` (macOS 26 Tahoe) by mSL project.
Reverse-engineered from the original arm64e binary.

## Project State (as of 2026-08-17)
- **All commands working**: status, get, set, --with/--without, disable, enable, clear
- **Builds clean** with `make` (debug) or `make DEBUG=0` (release)
- **Tested on Apple Silicon** — status output matches Apple's csrutil format

## Architecture Decisions
- **Headers live at `lib/lib<name>/include/<name>.h`** — standard C convention, flat, no namespace prefixes
- **Three static libraries**, each with its own Makefile:
  - `lib/libbootpolicy.a` — dlopen/dlsym wrapper for `/usr/lib/libbootpolicy.dylib` (20+ symbols)
  - `lib/libauthinstall.a` — dlopen/dlsym wrapper for ACM symbols from `libauthinstall.dylib`
  - `lib/libcsrutil.a` — core SIP logic (csr.c), depends on headers from the other two
- **CLI (`src/main.c`)** is pure UI — positional verb parsing (no getopt to avoid `--without`/`--with` conflicts)
- **XNU CSR_ALLOW_* bits** in `csr.h` are the authoritative flag definitions
- **`csr_flag_table[]`** maps bits to human names and `--without` argument strings
- Status reads via `csr_get_active_config()` kernel syscall + libbootpolicy for security mode

## Key Files
- `lib/libcsrutil/include/csrutil.h` — public API (csrutil_state_t, csrutil_status, csrutil_set_flags, error codes)
- `lib/libcsrutil/include/csr.h` — XNU CSR_ALLOW_* bit definitions
- `lib/libcsrutil/src/csr.c` — core SIP logic (314 lines)
- `lib/libbootpolicy/include/bootpolicy.h` — libbootpolicy interface (20+ functions, types, global accessors)
- `lib/libauthinstall/include/acm.h` — ACM API (context, credential, policy verify, authenticate)
- `src/main.c` — CLI entry point

## What's NOT Done
- `csrutil authenticate` — interactive ACM auth flow
- Bootability.framework / DiskManagement.framework integration (skeletal stubs in `src/`)
- Unit/integration tests
- Nonce lifecycle testing (`csrutil disable` with sudo on real hardware)

## Build
```bash
make              # debug
make DEBUG=0      # release
make clean && make  # full rebuild
```
Requires Xcode Command Line Tools (clang, ar). No other dependencies.

## Conventions
- C11 standard, `-Wall -Wextra -Werror`
- Copyright (c) 2025 mSL project — BSD-3-Clause licence
- Libraries use dlopen/dlsym pattern for dyld shared cache symbols
- No external dependencies beyond macOS system frameworks
