#ifndef BLZ_SYSLOG_H
#define BLZ_SYSLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>

#define SEPARATOR ' '
#define NIL '-'
#define QUOTE '"'
#define CLOSE_BRACKET ']'
#define OPEN_BRACKET '['
#define ESCAPE '\\'
#define PRI_VALUES_COUNT 24
#define EQUALS '='

static int PRI_VALUES[PRI_VALUES_COUNT] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184};

typedef struct syslog_extended_property_value_t {
  char* key;
  char* value;
} syslog_extended_property_value_t;

typedef struct syslog_extended_property_t {
  char* id;
  syslog_extended_property_value_t * pairs;
  size_t num_pairs;
} syslog_extended_property_t;

typedef struct syslog_message_t {
  const char* message;
  const char* syslog_version;

  int severity;
  int facility;
  int pri_value;

  const char* message_id;

  const char* hostname;
  const char* appname;

  const char* process_id;

  struct tm timestamp;
  enum syslog_type {
    RFC5424=5424
  } syslog_type;

  syslog_extended_property_t * structured_data;
  size_t structured_data_count;

  size_t message_length;
} syslog_message_t;

bool parse_syslog(const char*, syslog_message_t*);
void free_syslog_message_t(syslog_message_t * syslog_message);

#endif
