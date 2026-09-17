CC = clang
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=202405L
CFLAGS = -Wall -Werror -std=c23 -I.
LDFLAGS =
LDLIBS = -llua5.4 -lwebsockets

ifeq "${BUILD}" "Debug"
	CFLAGS += -g -O0
else
	CFLAGS += -O2
	CPPFLAGS += -DNDEBUG
	LDFLAGS += -s
endif

OBJS := nori/config.o \
		nori/server.o \
		nori/string.o \
		nori/util.o \
		src/main.o

.PHONY: clean

server: $(OBJS)
	mkdir -p bin
	$(CC) $^ -o bin/$@ $(LDFLAGS) $(LDLIBS)

clean:
	find . -name "*.o" -delete
	-rm -r bin
