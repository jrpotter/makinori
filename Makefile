CC = clang
CFLAGS = -Wall -Werror -std=c23 -I. -MMD
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L
LDFLAGS =
LDLIBS = -llua5.4 -lwebsockets

OBJS := $(patsubst %.c,%.o,$(wildcard **/*.c))

.PHONY: clean

server: $(OBJS)
	mkdir -p bin
	$(CC) $^ -o bin/$@ $(LDFLAGS) $(LDLIBS)

# Automatically track dependencies. The -MMD flag creates a .d file with
# object file dependencies at build time.
-include $(OBJS:.o=.d)

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
