#include "syslog.h"

#define SEPARATOR ' '
#define NIL '-'
#define QUOTE '"'
#define CLOSE_BRACKET ']'
#define OPEN_BRACKET '['
#define ESCAPE '\\'
#define PRI_VALUES_COUNT 24
#define EQUALS '='

static int PRI_VALUES[PRI_VALUES_COUNT] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184};

typedef struct syslog_parse_context_t {
  const char * message;
  int pointer;
} syslog_parse_context_t;

int parse_context_is_eol(syslog_parse_context_t * ctx) {
  return ctx->pointer >= strlen(ctx->message);
}

int parse_context_peek(syslog_parse_context_t * ctx, char* out) {
  if (parse_context_is_eol(ctx)) {
    return 0;
  }

  *out = ctx->message[ctx->pointer];

  return 1;
}

int parse_context_one(syslog_parse_context_t * ctx, char* out) {
  if (parse_context_peek(ctx, out)) {
    ctx->pointer++;
    return 1;
  }

  return 0;
}

size_t parse_context_next_until(syslog_parse_context_t * ctx, char until_char, char* writestr, bool include_eol) {
  const char* ptr = strchr(ctx->message + ctx->pointer, until_char);
  if (!ptr && !include_eol) {
    return 0;
  }

  size_t str_len = strlen(ctx->message);
  size_t newstr_len = 0;
  int old_pointer = ctx->pointer;

  if (include_eol) {
    // Create a new string
    newstr_len = str_len - ctx->pointer;
    ctx->pointer = str_len + 1;
  } else {
    size_t index = ptr - ctx->message;
    newstr_len = index - ctx->pointer;

    ctx->pointer = index + 1;
  }

  if (writestr) {
    // Copy the new string to the buffer
    strncpy(writestr, ctx->message + old_pointer, newstr_len);
  }

  return newstr_len;
}

size_t parse_context_next_until_with_escapes(syslog_parse_context_t * ctx, char until_char, char* writestr, bool evaluate_escapes, bool or_eol) {
  bool escaped = false;

  char c = 0;
  size_t i = 0;

  while (!parse_context_is_eol(ctx)) {
    i++;

    parse_context_one(ctx, &c);

    if (escaped) {
      if (c == QUOTE || c == CLOSE_BRACKET || c == ESCAPE) {
        escaped = false;
      } else {
        if (evaluate_escapes) {
          // only those 3 characters support escape characters, so we are supposed
          // to treat it as unescaped and NOT throw away the ESCAPE character.
          // See https://tools.ietf.org/html/rfc5424#section-6.3
          writestr[i] = ESCAPE;
        }
      }
    } else if (c == ESCAPE) {
      escaped = true;
      if (evaluate_escapes) {
        // to 'resolve' escapes into their escaped char we will
        // avoid appending the escape char to the builder.
        continue;
      }
    } else if (c == until_char) {
      // an unescaped until_char was found
      writestr[i] = 0;
			return i;
    }
    // Otherwise add the character
    writestr[i] = c;
  }

  if (or_eol) {
    return i;
  }

	// Just return a failure here because we didn't really get a string
	return 0;
}

int parse_context_get_structured_data_elements(syslog_parse_context_t * ctx, char* writestr, size_t * num_elements) {
  char start = 0;
  parse_context_peek(ctx, &start);

	*num_elements = 0;
	int intern_pointer = 0;

  if (start == 0) {
    // Structured data is nothing. We just want to advance to the separator
    parse_context_next_until(ctx, SEPARATOR, NULL, true);
    return 0;
  } else if (start != OPEN_BRACKET) {
    // Structured data must start with an open bracket, so this is invalid
    return 0;
  }

	char pk = 0;
  while (parse_context_peek(ctx, &pk) && pk == OPEN_BRACKET) {
    parse_context_one(ctx, &pk); // eat [
    // We need to find where structured data ends, but takes escapes into account
		int str_len = parse_context_next_until_with_escapes(ctx, CLOSE_BRACKET, &writestr[intern_pointer], false, false);
    if (str_len) {
      *num_elements = *num_elements + 1;
			// Increment this intern pointer by the length of the string returned + 1 for the
			// null terminator
			intern_pointer += str_len + 1;
    }
  }

  // If we are at EOL
  if (parse_context_is_eol(ctx)) {
    ctx->pointer++;
    return intern_pointer;
  }

  // If the last thing is a separator
  char buf = 0;
  parse_context_peek(ctx, &buf);
  if (buf == SEPARATOR) {
    ctx->pointer++;
    return intern_pointer;
  }

  // Final character is something else which is bad
  // This means structured data is bad
  *num_elements = 0;
	return intern_pointer;
}

syslog_parse_context_t create_parse_context(const char* raw_message) {
  // Pointer should automatically be initialized to 0
  syslog_parse_context_t ctx = { raw_message };
  return ctx;
}

int parse_structured_data_element(char* data_string, syslog_extended_property_t * property) {
  syslog_parse_context_t ctx = create_parse_context(data_string);

	// New write string time
	char* element_string = malloc(sizeof(char) * strlen(data_string) * 2);
	int intern_pointer = 0;

  // SD-ID
  int id_length = parse_context_next_until_with_escapes(&ctx, SEPARATOR, &element_string[intern_pointer], true, true);
  if (!id_length) {
    return 0;
  }

	property->id = &element_string[intern_pointer];

	// Add one for the null terminator
	intern_pointer += id_length + 1;

  if (parse_context_is_eol(&ctx)) {
    // This means the entire thing is the sd_id, as in
    // [exampleSDID@32473]
    // so it gets an empty value set and we're done.
    property->pairs = NULL;
    return 1;
  }

	// @todo Max 12 elements here. We need to make this bigger
  property->pairs = (syslog_extended_property_value_t*) malloc(sizeof(syslog_extended_property_value_t) * 12 + 1);

  size_t num_elements = 0;
  // FIX IT FIX IT FIX IT SHOULD BE DOING INTERNING HERE @TODO
  while (!parse_context_is_eol(&ctx)) {
		int key_len = parse_context_next_until(&ctx, EQUALS, &element_string[intern_pointer], false);
    if (!key_len) {
      // Invalid because we need a key and value
      break;
    }

    // Value must start with a quote
    char quote = 0;
    if (!parse_context_one(&ctx, &quote) || quote != QUOTE) {
      // needs to be a quote
      break;
    }

		char* key = &element_string[intern_pointer];

		intern_pointer += key_len + 1;

    int val_len = parse_context_next_until_with_escapes(&ctx, QUOTE, &element_string[intern_pointer], true, false);
    if (!val_len) {
      // No ending quote? What's wrong with you! courtsey of @dareed
      break;
    }

    num_elements++;

		char* value = &element_string[intern_pointer];

		intern_pointer += val_len + 1;

    property->pairs[num_elements - 1] = (syslog_extended_property_value_t) {key, value};

    if (!parse_context_is_eol(&ctx)) {
      char next = 0;
      if (!parse_context_one(&ctx, &next) || next != SEPARATOR) {
        break;
      }
    }
  }  // End while loop

  property->num_pairs = num_elements;

  return 1;
}

syslog_extended_property_t * get_structured_data(char* structured_data_elements, size_t num_elements) {
  // the message may contain multiple structured data parts, as in:
  // [exampleSDID@32473 iut="3" eventSource="Application" eventID="1011"][examplePriority@32473 class="high"]
  // in which case we are given each separately within the list array.
  syslog_extended_property_t * properties = (syslog_extended_property_t*) malloc(sizeof(syslog_extended_property_t) * 24 + 1);

  int ep_num = 0;
	int last_string_size = 0;

  for (size_t i = 0; i < num_elements; i++) {
    char* st_element = &structured_data_elements[last_string_size];
    if (parse_structured_data_element(st_element, &properties[ep_num])) {
      ep_num++;
    }
		last_string_size = strlen(st_element);
  }

  return properties;
}

int get_facility_id(int pri_value) {
  // given a pri-value from a SysLog entry, which is Facility*8+Severity,
  // return the Facility value portion. Which is given by the maximum _priValue
  // that is less than priValue.
  for (int i = PRI_VALUES_COUNT - 1; i >= 0; i--) {
    // find the first item in the list, starting from the end, that is less than or equal to the priValue.
    int val = PRI_VALUES[i];
    if (val <= pri_value) {
      return val;
    }
  }

  return 0;
}

int parse_iso_8601(const char* datestring, struct tm* tptr) {
  int y,M,d,h,m;
  float s;
  int tzh = 0, tzm = 0;
  if (6 < sscanf(datestring, "%d-%d-%dT%d:%d:%f%d:%dZ", &y, &M, &d, &h, &m, &s, &tzh, &tzm)) {
    if (tzh < 0) {
      tzm = -tzm;    // Fix the sign on minutes.
    }
  }

  tptr->tm_year = y - 1900; // Year since 1900
  tptr->tm_mon = M - 1;     // 0-11
  tptr->tm_mday = d;        // 1-31
  tptr->tm_hour = h;        // 0-23
  tptr->tm_min = m;         // 0-59
  tptr->tm_sec = (int)s;    // 0-61

  return 1;
}

char* filter_nil(char* s) {
  if (strlen(s) == 1 && s[0] == 0) {
    return "";
  }

  return s;
}

bool parse_syslog_message_t(const char* raw_message, syslog_message_t * message) {
  syslog_parse_context_t ctx = create_parse_context(raw_message);

  char* intern = malloc((sizeof(char) * strlen(raw_message) * 2) + 8);

  // --- PRI
  char buf = 0;
  if (!parse_context_one(&ctx, &buf)) {
    return false;
  }

  if (buf != '<') {
    return false;
  }

  int intern_pointer = 0;

  int pri_val_length = parse_context_next_until(&ctx, '>', &intern[0], false);
  // We do not need the position here. Just check if it worked
  if (!pri_val_length) {
    return false;
  }

  // Don't forget the null terminator
  intern_pointer += pri_val_length + 1;

  // Again... for clarity's sake
  int pri_value = atoi(&intern[0]);

  if (pri_value < 0 || pri_value > 191) {
    return false;
  }

  int facility_id = get_facility_id(pri_value);
  // Omg we got our first syslog value
  message->pri_value = pri_value;
  message->facility = facility_id / 8;
  message->severity = pri_value - facility_id;

  // We are actually done with that string in the intern buffer now

  // --- VERSION
  int syslog_version_length = parse_context_next_until(&ctx, SEPARATOR, &intern[intern_pointer], false);
  if (!syslog_version_length || syslog_version_length > 2) {
		// We do not need to deallocate the memory because none of the previous
		// stuff has been malloc'd if this returned null
		return false;
  }

  message->syslog_version = &intern[intern_pointer];

  intern_pointer += syslog_version_length + 1;

  // --- TIMESTAMP
  int timestamp_length = parse_context_next_until(&ctx, SEPARATOR, &intern[intern_pointer], false);
  if (!timestamp_length) {
		return false;
  }

  char* timestamp = &intern[intern_pointer];

	if (timestamp_length == 1 && timestamp[0] == 0) {
    // This means we want to get the current time
    time_t rawtime;
    time(&rawtime);

    message->timestamp = *localtime(&rawtime);
  } else {
    parse_iso_8601(timestamp, &message->timestamp);
  }

  intern_pointer += timestamp_length + 1;

	// We do not need the timestamp anymore either

	// --- HOSTNAME
  int hostname_length = parse_context_next_until(&ctx, SEPARATOR, &intern[intern_pointer], false);
  if (!hostname_length) {
		return false;
  }

  message->hostname = filter_nil(&intern[intern_pointer]);

  intern_pointer += hostname_length + 1;

  // --- APP-NAME
  int appname_length = parse_context_next_until(&ctx, SEPARATOR, &intern[intern_pointer], false);
  if (!appname_length) {
		return false;
  }

  message->appname = filter_nil(&intern[intern_pointer]);

  intern_pointer += appname_length + 1;

  // --- PROCID
  // surprisingly, can be a string up to 128 chars
  int process_id_length = parse_context_next_until(&ctx, SEPARATOR, &intern[intern_pointer], false);
  if (!process_id_length) {
		return false;
  }

  message->process_id = filter_nil(&intern[intern_pointer]);

  intern_pointer += appname_length + 1;

  // --- MSGID
  int message_id_length = parse_context_next_until(&ctx, SEPARATOR, &intern[intern_pointer], false);
  if (!message_id_length) {
		return false;
  }

  message->message_id = filter_nil(&intern[intern_pointer]);

  intern_pointer += message_id_length + 1;

  size_t num_structured_data;
	int buf_size = parse_context_get_structured_data_elements(&ctx, &intern[intern_pointer], &num_structured_data);

  if (num_structured_data < 1) {
    message->structured_data = NULL;
    message->structured_data_count = 0;
  } else {
    message->structured_data_count = num_structured_data;
    message->structured_data = get_structured_data(&intern[intern_pointer], num_structured_data);
  }

	// No matter what we need to increment the intern pointer here. Because we used the string.
	intern_pointer += buf_size;

  // --- MSG
  // Rest of the data is the message
  if (parse_context_is_eol(&ctx)) {
    message->message = NULL;
  } else {
    parse_context_next_until(&ctx, '\0', &intern[intern_pointer], true);
    message->message = &intern[intern_pointer];
  }

	return true;
}

void free_syslog_extended_property_value_t(syslog_extended_property_value_t * property_value) {
	free((char*) property_value->key);
	free((char*) property_value->value);
}

void free_syslog_extended_property_t(syslog_extended_property_t * extended_property) {
	// Iterate over that now
	if (extended_property->num_pairs > 0) {
		// Okay... we have some pairs
		for (size_t i = 0; i < extended_property->num_pairs; i++) {
			free_syslog_extended_property_value_t(&extended_property->pairs[i]);
		}

		free(extended_property->pairs);
	}

	free((char*) extended_property->id);
}

void free_syslog_message_t(syslog_message_t * msg) {
	// Essentially we need to iterate over the fields that have been malloc'd and free them
	// Just a helper utility since the structure is slightly complicated

	if (msg->message) {
		free((char*) msg->message);
	}

	if (msg->syslog_version) {
		free((char*) msg->syslog_version);
	}

	if (msg->message_id) {
		free((char*) msg->message_id);
	}

	if (msg->hostname) {
		free((char*) msg->hostname);
	}

	if (msg->appname) {
		free((char*) msg->appname);
	}

	if (msg->process_id) {
		free((char*) msg->process_id);
	}

	if (msg->structured_data) {
		for (size_t i = 0; i < msg->structured_data_count; i++) {
			free_syslog_extended_property_t(&msg->structured_data[i]);
		}
		free(msg->structured_data);
	}

	msg = NULL;
}
