AM_CFLAGS= -Wall -g -Wno-unused-function -I../src
AM_LDFLAGS=

TESTS = test_syslog

check_PROGRAMS = test_syslog
test_syslog_SOURCES = clar.c char.h main.c test.c test.h syslog_message_with_structured_data.c syslog_message.c
test_syslog_LDADD = ../src/libsyslog.a

DISTCLEANFILES= clar.suite .clarcache
