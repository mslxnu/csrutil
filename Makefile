# Makefile — csrutil open-source reimplementation (macOS 26 / Tahoe)
#
# Build architecture:
#   lib/libbootpolicy.a   — dlopen/dlsym wrapper for libbootpolicy.dylib
#   lib/libauthinstall.a  — dlopen/dlsym wrapper for libauthinstall.dylib (ACM)
#   lib/libcsrutil.a      — core SIP API (csr.c), links headers from above
#   csrutil               — CLI binary, links all libraries + system frameworks
#
# Copyright (c) 2025 mSL project — BSD-3-Clause licence.

# ── Toolchain ───────────────────────────────────────────────────────

CC       = clang
CFLAGS   = -Wall -Wextra -Werror -std=c11
OBJCFLAGS = -Wall -Wextra -Werror -std=objc11
AR       = ar
ARFLAGS  = rcs

# ── Debug / Release ─────────────────────────────────────────────────

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS   += -g -O0 -DDEBUG
    OBJCFLAGS += -g -O0 -DDEBUG
else
    CFLAGS   += -O2 -DNDEBUG
    OBJCFLAGS += -O2 -DNDEBUG
endif

# ── Include paths ───────────────────────────────────────────────────
#
# The binary needs headers from all three libraries and from
# PrivateFrameworks/ for the Bootability/DiskManagement wrappers.

LIB_INCLUDES = \
    -Ilib/libcsrutil/include \
    -Ilib/libbootpolicy/include \
    -Ilib/libauthinstall/include

FW_INCLUDES  = -IPrivateFrameworks

ALL_INCLUDES = -Isrc $(LIB_INCLUDES) $(FW_INCLUDES)

# ── Libraries (built by lib/Makefile) ──────────────────────────────

LIB_DIR      = lib
LIB_BOOTPOLICY = $(LIB_DIR)/libbootpolicy/libbootpolicy.a
LIB_AUTHINSTALL = $(LIB_DIR)/libauthinstall/libauthinstall.a
LIB_CSRUTIL    = $(LIB_DIR)/libcsrutil/libcsrutil.a

# ── Binary source files ─────────────────────────────────────────────

C_SRCS   = src/main.c src/log.c src/Bootability.c src/DiskManagement.c
C_OBJS   = $(C_SRCS:.c=.o)

# ── Target ──────────────────────────────────────────────────────────

TARGET = csrutil

.PHONY: all clean install uninstall libs test debug run

all: libs $(TARGET)

# ── Build libraries ─────────────────────────────────────────────────

libs:
	@$(MAKE) -C $(LIB_DIR)

# ── Build binary ────────────────────────────────────────────────────

$(TARGET): $(C_OBJS) $(LIB_CSRUTIL) $(LIB_BOOTPOLICY) $(LIB_AUTHINSTALL)
	$(CC) $(C_OBJS) \
	    $(LIB_CSRUTIL) $(LIB_BOOTPOLICY) $(LIB_AUTHINSTALL) \
	    -o $@ \
	    -framework CoreFoundation -framework DiskArbitration \
	    -framework IOKit -lobjc -ldl
	@echo "Built $(TARGET)"

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(ALL_INCLUDES) -c $< -o $@

# ── Housekeeping ────────────────────────────────────────────────────

clean:
	@$(MAKE) -C $(LIB_DIR) clean
	rm -f $(C_OBJS) $(TARGET)

install: $(TARGET)
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)

test: $(TARGET)
	./$(TARGET) status

debug: $(TARGET)
	lldb ./$(TARGET)

run: $(TARGET)
	./$(TARGET)
