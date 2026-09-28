# ==================================================================================
# General

CC           = clang
CFLAGS       = -Wall -Werror -std=c23 -Iinclude
CPPFLAGS     = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_GNU_SOURCE
LDFLAGS      =
LDLIBS       =

SPHINXOPTS  ?=
SPHINXBUILD ?= sphinx-build
SOURCEDIR    = docs
BUILDDIR     = docs/_build
MODE        ?= help

# shell/find searches arbitrarily deep unlike wildcard.
OBJS         = $(patsubst %.c,%.o,$(shell find ./src -name "*.c"))

# ==================================================================================
# Configuration

CFLAGS += -I/usr/include/lua5.5
LDLIBS += -lwebsockets -llua5.5

# ==================================================================================
# Dependencies

# Automatically track dependencies. The -MMD flag creates a .d file with
# object file dependencies at build time. We then -include (at the bottom) any
# definition files that are generated on subsequent builds.
CFLAGS += -MMD

# ==================================================================================
# Hardening

# Force the compiler to allocate one page of stack space at a time, immediately
# accessing the page after allocation. This ensures no means of jumping over the
# guard page setup on our coroutine stacks.
CFLAGS += -fstack-clash-protection

ifeq ($(BUILD_TYPE),Debug)
	CFLAGS += -g -O0
else ifeq ($(BUILD_TYPE),Release)
	CFLAGS += -O2
	CPPFLAGS += -DNDEBUG
	LDFLAGS += -s
endif

# ==================================================================================
# Recipes

all: bin/cmdline-usage

bin/%: examples/%.o $(OBJS)
	mkdir -p bin
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

docs: MODE=html
docs: docs/_build/server

docs/_build/server: docs/main.o $(OBJS) sphinx
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) $(LDLIBS)

sphinx:
	@$(SPHINXBUILD) -M $(MODE) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

clean: MODE=clean
clean: sphinx
	find . -name "*.d" -delete
	find . -name "*.o" -delete
	[ -d bin ] && rm -r bin

.PHONY: all clean docs sphinx

# Include at the end to avoid interfering with default rules.
-include $(OBJS:.o=.d)
