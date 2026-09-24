CC = clang
CFLAGS = -Wall -Werror -std=c23 -Iinclude
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_GNU_SOURCE
LDFLAGS =
LDLIBS = -lwebsockets

# Lua specific dependencies. Update depending on your environment.
# TODO: For installation, should look into something like `configure`.
CFLAGS += -I/usr/include/lua5.4
LDLIBS += -llua5.4

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
ASMS := $(patsubst %.c,%.s,$(shell find ./src -name "*.c"))
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

# Generate all the corresponding assembly files instead.
asm: $(ASMS)

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -S $<

clean:
	find . -name "*.d" -delete
	find . -name "*.o" -delete
	find . -name "*.s" -delete
	-rm -r bin
