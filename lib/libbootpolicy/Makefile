# lib/libbootpolicy/Makefile — libbootpolicy.dylib wrapper
#
# Produces: libbootpolicy.a
# Contains: bootpolicy.c (dlopen/dlsym wrapper for /usr/lib/libbootpolicy.dylib)
#
# This is a static archive that resolves symbols at runtime, not link time.
# The final binary still needs -ldl at link time for dlopen/dlsym.
#
# Copyright (c) 2025 mSL project — BSD-3-Clause licence.

CC       ?= clang
CFLAGS   += -Wall -Wextra -Werror -std=c11 -Iinclude
AR       ?= ar
ARFLAGS   = rcs

DEBUG    ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

SRC      = src/bootpolicy.c
OBJ      = $(SRC:.c=.o)
TARGET   = libbootpolicy.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
