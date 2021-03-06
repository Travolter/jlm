# Copyright 2014 Nico Reißmann <nico.reissmann@gmail.com>
# See COPYING for terms of redistribution.

define HELP_TEXT
clear
echo "Makefile for the JLM compiler"
echo "Version 1.1 - 2019-11-26"
endef
.PHONY: help
help:
	@$(HELP_TEXT)
	@$(HELP_TEXT_JLM)
	@echo ""
	@echo "submodule              Initializes all the dependent git submodules"
	@echo "all                    Compile jlm in debug mode, and run unit and C tests"
	@echo "clean                  Calls clean for jive and jlm"
	@$(HELP_TEXT_JIVE)

JLM_ROOT ?= .
JIVE_ROOT ?= $(JLM_ROOT)/external/jive

include $(JLM_ROOT)/Makefile.sub
include $(JLM_ROOT)/tests/Makefile.sub
ifneq ("$(wildcard $(JIVE_ROOT)/Makefile.sub)","")
include $(JIVE_ROOT)/Makefile.sub
endif

LLVMCONFIG ?= llvm-config

.PHONY: all
all: jlm-debug check

.PHONY: submodule
submodule:
	git submodule update --init --recursive

%.la: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.la: %.c
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.a:
	rm -f $@
	ar clqv $@ $^
	ranlib $@

.PHONY: clean
clean: jive-clean jlm-clean

ifeq ($(shell if [ -e .Makefile.override ] ; then echo yes ; fi),yes)
include .Makefile.override
endif
