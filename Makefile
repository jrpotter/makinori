# ==================================================================================
# Variables

LUA_SRC    := lib/lua
LWS_SRC    := lib/libwebsockets

AR         := ar
CC         := clang
CFLAGS     := -Wall -Werror -std=c23 -Iinclude
CPPFLAGS   := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_GNU_SOURCE
LDFLAGS    :=
LDLIBS     := -lm

BUILD_BIN  ?= build/bin
BUILD_LIB  ?= build/lib
BUILD_TYPE ?= Debug

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
# General

.PHONY: all clean docs examples lib prune sphinx test

all: examples lib

# ==================================================================================
# Libraries

MAKINORI := $(BUILD_LIB)/libmakinori.a
LUA      := $(BUILD_LIB)/liblua.a
LWS      := $(BUILD_LIB)/libwebsockets.a

lib: $(MAKINORI) $(LUA) $(LWS)

OBJS := $(patsubst %.c,%.o,$(shell find ./src -name "*.c"))

$(MAKINORI): $(OBJS)
	mkdir -p $(BUILD_LIB)
	$(AR) rcs $@ $(OBJS)

$(OBJS): CFLAGS += -I$(LUA_SRC) -I$(LWS_SRC)/build/include
$(OBJS): %.o: %.c $(LUA) $(LWS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(LUA): export CC=clang
$(LUA): export CWARNGCC=
$(LUA):
	mkdir -p $(BUILD_LIB)
	cd $(LUA_SRC) && $(MAKE) -e liblua.a
	cp $(LUA_SRC)/liblua.a $@

$(LWS):
	mkdir -p $(BUILD_LIB)
	cd $(LWS_SRC) && cmake \
		-G 'Unix Makefiles' \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER=$(CC) \
		-DLWS_CLIENT_HTTP_PROXYING=OFF \
		-DLWS_CTEST_INTERNET_AVAILABLE=OFF \
		-DLWS_WITHOUT_BUILTIN_SHA1=ON \
		-DLWS_WITHOUT_CLIENT=ON \
		-DLWS_WITHOUT_TESTAPPS=ON \
		-DLWS_WITHOUT_TEST_CLIENT=ON \
		-DLWS_WITHOUT_TEST_PING=ON \
		-DLWS_WITHOUT_TEST_SERVER=ON \
		-DLWS_WITHOUT_TEST_SERVER_EXTPOLL=ON \
		-DLWS_WITH_DIR=ON \
		-DLWS_WITH_DLO=OFF \
		-DLWS_WITH_FILE_OPS=OFF \
		-DLWS_WITH_GZINFLATE=OFF \
		-DLWS_WITH_HTTP_BASIC_AUTH=OFF \
		-DLWS_WITH_HTTP_DIGEST_AUTH=OFF \
		-DLWS_WITH_JPEG=OFF \
		-DLWS_WITH_JSONRPC=OFF \
		-DLWS_WITH_LEJP=OFF \
		-DLWS_WITH_LEJP_CONF=ON \
		-DLWS_WITH_LHP=OFF \
		-DLWS_WITH_LIBCAP=OFF \
		-DLWS_WITH_MINIMAL_EXAMPLES=OFF \
		-DLWS_WITH_SECURE_STREAMS=OFF \
		-DLWS_WITH_SHARED=OFF \
		-DLWS_WITH_SSL=OFF \
		-DLWS_WITH_SYS_SMD=OFF \
		-DLWS_WITH_SYS_STATE=OFF \
		-DLWS_WITH_UPNG=OFF \
		-B build
	cd $(LWS_SRC)/build && $(MAKE)
	cp $(LWS_SRC)/build/lib/libwebsockets.a $@

# ==================================================================================
# Examples

examples: $(patsubst examples/%.c,$(BUILD_BIN)/example-%,$(wildcard examples/*.c))

LDFLAGS += -L$(BUILD_LIB)
LDLIBS += -lmakinori -llua -lwebsockets

$(BUILD_BIN)/example-%: examples/%.o $(OBJS) lib
	mkdir -p $(BUILD_BIN)
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) $(LDLIBS)

# ==================================================================================
# Documentation

SPHINXOPTS  ?=
SPHINXBUILD ?= sphinx-build
SOURCEDIR    = docs
BUILDDIR     = docs/_build
MODE        ?= help

docs: $(BUILD_BIN)/docs

$(BUILD_BIN)/docs: MODE=html
$(BUILD_BIN)/docs: docs/main.o lib sphinx
	mkdir -p $(BUILD_BIN)
	$(CC) $< -o $@ $(LDFLAGS) $(LDLIBS)

sphinx:
	@$(SPHINXBUILD) -M $(MODE) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# ==================================================================================
# Tests

TESTS := $(patsubst %.c,%.o,$(shell find tests -name "*.c"))

test: $(BUILD_BIN)/test
	$(BUILD_BIN)/test

$(BUILD_BIN)/test: $(TESTS) lib $(TESTS)
	mkdir -p $(BUILD_BIN)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test.c -o $@ $(LDFLAGS) $(LDLIBS)

# ==================================================================================
# Cleanup

clean:
	find . -name "*.d" -not -path "./lib/*" -delete
	find . -name "*.o" -not -path "./lib/*" -delete
	if [ -d build ]; then rm -r build; fi
	if [ -d docs/_build ]; then rm -r docs/_build; fi

prune: clean
	cd $(LUA_SRC) && $(MAKE) clean
	if [ -d $(LWS_SRC)/build ]; then rm -r $(LWS_SRC)/build; fi

# Include at the end to avoid interfering with other rules.
-include $(OBJS:.o=.d)
