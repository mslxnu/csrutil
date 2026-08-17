# lib/Makefile — Orchestrates building all sub-libraries.
#
# Each sub-directory produces a static archive (.a) that the root
# Makefile links against when building the csrutil binary.
#
# Build order matters: libcsrutil depends on headers from
# libbootpolicy and libauthinstall, but NOT their object files
# (those are resolved at runtime via dlsym).
#
# Copyright (c) 2025 mSL project — BSD-3-Clause licence.

SUBDIRS = libbootpolicy libauthinstall libcsrutil

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	@echo "── Building $@ ──"
	$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
