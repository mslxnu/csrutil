# lib/libcsrutil/Makefile — Core SIP library
#
# Produces: libcsrutil.a
# Contains: csr.c (high-level SIP API using libbootpolicy + acm)
#
# Copyright (c) 2025 mSL project — BSD-3-Clause licence.

CC       ?= clang
CFLAGS   += -Wall -Wextra -Werror -std=c11 \
            -Iinclude \
            -I../libbootpolicy/include \
            -I../libauthinstall/include
AR       ?= ar
ARFLAGS   = rcs

DEBUG    ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

SRC      = src/csr.c
OBJ      = $(SRC:.c=.o)
TARGET   = libcsrutil.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
