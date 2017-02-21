PROGRAM_NAME= syslog

CC= gcc
CFLAGS= -Wall
SRC= $(wildcard src/*.c)
OBJS= $(subst .c,.o,$(SRC))
LDFLAGS= -shared

TESTSRC= $(wildcard tests/*.c)

.PHONY: tests

all: $(SRC) $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJS)
	$(CC) -g $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

print-%  : ; @echo $* = $($*)

%.o : %.c
	$(CC) -g $(CFLAGS) -c $< -o $@

clean:
	rm src/*.o tests/*.o

tests: $(TESTSRC)
	$(CC) -I src/ $(CFLAGS) $(SRC) $(TESTSRC) -o runtests
