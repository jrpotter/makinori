CC = clang
CFLAGS = -Wall -Werror -std=c23 -Iinclude -MMD
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L -D_GNU_SOURCE
LDFLAGS =
LDLIBS = -llua5.4 -lwebsockets

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

# Automatically track dependencies. The -MMD flag creates a .d file with
# object file dependencies at build time.
-include $(OBJS:.o=.d)

# Generate all the corresponding assembly files instead.
asm: $(ASMS)

%.s: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -S $<

clean:
	find . -name "*.d" -delete
	find . -name "*.o" -delete
	find . -name "*.s" -delete
	-rm -r bin
