# ==================================================================================
# General

LUA_DIR     := lib/lua
LWS_DIR     := lib/libwebsockets

CC          := clang
CFLAGS      := -Wall -Werror -std=c23 -Iinclude -Ilib/lua
CPPFLAGS    := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_GNU_SOURCE
LDFLAGS     := -L$(LUA_DIR) -L$(LWS_DIR)/build/lib
LDLIBS      := -lm -llua -lwebsockets

BUILD_TYPE  ?= Debug

ifeq ($(BUILD_TYPE),Debug)
	CFLAGS += -g -O0
else ifeq ($(BUILD_TYPE),Release)
	CFLAGS += -O2
	CPPFLAGS += -DNDEBUG
	LDFLAGS += -s
endif

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

# ==================================================================================
# makinori

# shell/find searches arbitrarily deep unlike wildcard.
OBJS         = $(patsubst %.c,%.o,$(shell find ./src -name "*.c"))

.PHONY: all clean docs lua lws sphinx

all: bin/cmdline-usage

bin/%: examples/%.o $(OBJS) lua lws
	mkdir -p bin
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) $(LDLIBS)

# Include at the end to avoid interfering with default rules.
-include $(OBJS:.o=.d)

clean: MODE=clean
clean: sphinx
	cd $(LUA_DIR) && $(MAKE) clean
	if [ -d $(LWS_DIR)/build ]; then rm -r $(LWS_DIR)/build; fi
	find . -name "*.d" -delete
	find . -name "*.o" -delete
	if [ -d bin ]; then rm -r bin; fi

# ==================================================================================
# Dependencies

lua: export CC=clang
lua: export CWARNGCC=
lua:
	cd $(LUA_DIR) && $(MAKE) -e liblua.a

lws:
	cd $(LWS_DIR) && cmake \
		-G 'Unix Makefiles' \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DLWS_WITH_MINIMAL_EXAMPLES=OFF \
		-DLWS_WITH_SSL=OFF \
		-B build
	cd $(LWS_DIR)/build && $(MAKE)

# ==================================================================================
# Documentation

SPHINXOPTS  ?=
SPHINXBUILD ?= sphinx-build
SOURCEDIR    = docs
BUILDDIR     = docs/_build
MODE        ?= help

docs: MODE=html
docs: docs/_build/server

docs/_build/server: docs/main.o $(OBJS) sphinx
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) $(LDLIBS)

sphinx:
	@$(SPHINXBUILD) -M $(MODE) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
