# csrutil

Open-source reimplementation of Apple's `/usr/bin/csrutil` for macOS 26 (Tahoe).

Reads and modifies System Integrity Protection (SIP) configuration by calling into
XNU syscalls, `libbootpolicy.dylib`, and `libauthinstall.dylib` (ACM) at runtime.

## Project Layout

```
csrutil/
├── Makefile                          # root build — libs then binary
├── lib/
│   ├── Makefile                      # orchestrates sub-library builds
│   ├── libbootpolicy/
│   │   ├── include/bootpolicy.h      # dlopen/dlsym API for libbootpolicy.dylib
│   │   ├── src/bootpolicy.c
│   │   └── Makefile
│   ├── libauthinstall/
│   │   ├── include/acm.h             # dlopen/dlsym API for ACM symbols
│   │   ├── src/acm.c
│   │   └── Makefile
│   └── libcsrutil/
│       ├── include/
│       │   ├── csr.h                 # XNU CSR_ALLOW_* bit definitions
│       │   └── csrutil.h             # public API (csrutil_status, csrutil_set_flags, …)
│       ├── src/csr.c
│       └── Makefile
└── src/
    ├── main.c                        # CLI entry point (positional verb parsing)
    ├── log.c / log.h                 # logging utility
    ├── Bootability.c                 # Bootability.framework wrapper (skeletal)
    └── DiskManagement.c              # DiskManagement.framework wrapper (skeletal)
```

**Headers live at `lib/lib<name>/include/<name>.h`** — standard C library convention.
No namespace prefixes, no redundant nesting.

## Building

```
make              # debug build (default)
make DEBUG=0      # release build (-O2)
make clean
make install      # copies to /usr/local/bin
```

Requires Xcode Command Line Tools (`clang`, `ar`).

## Usage

```
csrutil status
csrutil get csr7
csrutil set csr0 csr1
csrutil --without kext
csrutil --with kext
csrutil disable   # requires sudo
csrutil enable    # requires sudo
csrutil clear     # reset to factory defaults (requires sudo)
```

## Library Architecture

| Library | Role | How It Works |
|---|---|---|
| `libbootpolicy` | Wraps `/usr/lib/libbootpolicy.dylib` | `dlopen` + `dlsym` at init; 20+ symbols resolved |
| `libauthinstall` | Wraps ACM (Apple Credential Manager) from `libauthinstall.dylib` | Same `dlopen`/`dlsym` pattern |
| `libcsrutil` | Core SIP logic | Reads flags via `syscsr`, writes via `csrutil_set_flags()`, uses the above two for nonce/credential operations |

The CLI binary (`src/main.c`) is pure UI — it parses verbs and delegates to the libcsrutil API.

## Status

- [x] `status` — reads all 14 CSR bits + security mode
- [x] `get <csrN>` — read individual flag
- [x] `set <csr0> ...` — set flags via nonce-based commit
- [x] `--without` / `--with` — toggle flags by name
- [x] `disable` / `enable` / `clear` — convenience commands (sudo)
- [ ] `authenticate` — interactive ACM auth flow
- [ ] Bootability / DiskManagement framework integration

## Licence

BSD-3-Clause. See source headers.
