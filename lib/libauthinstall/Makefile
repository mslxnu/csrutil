# lib/libauthinstall/Makefile — ACM (Apple Credential Manager) stub
#
# Produces: libauthinstall.a
# Contains: acm_stub.c (empty — all real symbols are dlsym'd at runtime)
#
# The headers in include/ declare the ACM API.  The actual symbols
# are resolved via dlsym() against /usr/lib/libauthinstall.dylib
# at runtime.  This archive exists solely to satisfy the linker.
#
# Copyright (c) 2025 mSL project — BSD-3-Clause licence.

CC       ?= clang
CFLAGS   += -Wall -Wextra -Werror -std=c11 -fblocks -Iinclude
AR       ?= ar
ARFLAGS   = rcs

DEBUG    ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

SRC      = src/acm.c src/la_auth.c
OBJ      = $(SRC:.c=.o)
TARGET   = libauthinstall.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
