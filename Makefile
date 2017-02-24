PROGRAM_NAME= libsyslog.so

CC= gcc
CFLAGS= -Wall -g
TEST_CFLAGS= -g -Wno-unused-function
BENCH_FLAGS= -O2
SRC= $(wildcard src/*.c)
OBJS= $(subst .c,.o,$(SRC))
HEADERS= $(wildcard src/*.h)
LDFLAGS= -shared
LIBTOOL= libtool
PY= python

TESTSRC= $(wildcard tests/**/*.c)
CLARSRC = $(wildcard tests/*.c)

all: $(SRC) $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJS) $(HEADERS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

print-%  : ; @echo $* = $($*)

%.o : %.c
	$(CC) -fPIC $(CFLAGS) -c $< -o $@

clean:
	rm -rf src/*.o tests/*.o syslog.a tests/clar.suite tests/.clarcache

test: clar.suite $(PROGRAM_NAME) $(TESTSRC)
	$(CC) -Itests/ -Isrc/ $(TEST_CFLAGS) $(SRC) $(CLARSRC) $(TESTSRC) -o runtests
	./runtests

benchmark: $(PROGRAM_NAME) bench/benchmark.c
	$(CC) -Isrc/ $(BENCH_FLAGS) $(SRC) bench/benchmark.c -o benchmark

install: $(PROGRAM_NAME)
	$(LIBTOOL) --mode=install cp $(PROGRAM_NAME) /usr/local/lib/$(PROGRAM_NAME)
	mkdir -p /usr/local/include/blizzard
	cp src/syslog.h /usr/local/include/blizzard/syslog.h

clar.suite:
	$(PY) tests/generate.py tests
