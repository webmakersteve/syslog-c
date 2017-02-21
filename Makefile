PROGRAM_NAME= libsyslog.so

CC= gcc
CFLAGS= -Wall -g
SRC= $(wildcard src/*.c)
OBJS= $(subst .c,.o,$(SRC))
LDFLAGS= -shared
LIBTOOL= libtool

TESTSRC= $(wildcard tests/*.c)

all: $(SRC) $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

print-%  : ; @echo $* = $($*)

%.o : %.c
	$(CC) -fPIC $(CFLAGS) -c $< -o $@

clean:
	rm -rf src/*.o tests/*.o syslog.a

test: $(TESTSRC)
	$(CC) -I src/ $(CFLAGS) $(SRC) $(TESTSRC) -o runtests
	./runtests

install: $(PROGRAM_NAME)
	$(LIBTOOL) --mode=install cp $(PROGRAM_NAME) /usr/local/lib/$(PROGRAM_NAME)
	mkdir -p /usr/local/include/blizzard
	cp src/syslog.h /usr/local/include/blizzard/syslog.h
