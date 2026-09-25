# ==================================================================================
# General

CC = clang
CFLAGS = -Wall -Werror -std=c23 -Iinclude
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_GNU_SOURCE
LDFLAGS =
LDLIBS =

# ==================================================================================
# Configuration

CFLAGS += -I/usr/include/lua5.4
LDLIBS += -lwebsockets -llua5.4

# CPPFLAGS += -D_MN_REQUEST_MAX_PATH_LEN=2048
# CPPFLAGS += -D_MN_REQUEST_MAX_CAPTURES=16
# CPPFLAGS += -D_MN_REQUEST_MAX_QUERY_PARAMS=16

# ==================================================================================
# Dependencies

# Automatically track dependencies. The -MMD flag creates a .d file with object
# file dependencies at build time. We then -include any definition files that
# are generated on subsequent builds.
CFLAGS += -MMD

-include $(OBJS:.o=.d)

# Force the compiler to allocate one page of stack space at a time, immediately
# accessing the page after allocation. This ensures no means of jumping over the
# guard page setup on our coroutine stacks.
CFLAGS += -fstack-clash-protection

# shell/find searches arbitrarily deep unlike wildcard.
OBJS += $(patsubst %.c,%.o,$(shell find ./src -name "*.c"))

.PHONY: clean

server: $(OBJS)
	mkdir -p bin
	$(CC) $^ -o bin/$@ $(LDFLAGS) $(LDLIBS)

debug: server
debug: CFLAGS += -g -O0

release: server
release: CFLAGS += -O2
release: CPPFLAGS += -DNDEBUG
release: LDFLAGS += -s

clean:
	find . -name "*.d" -delete
	find . -name "*.o" -delete
	-rm -r bin
