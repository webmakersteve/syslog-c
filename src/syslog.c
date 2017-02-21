#include "syslog.h"

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

char* parse_context_next_until(syslog_parse_context_t * ctx, char until_char, bool include_eol) {
  const char* ptr = strchr(ctx->message + ctx->pointer, until_char);
  if (!ptr && !include_eol) {
    return NULL;
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

  // Allocate the new string length sized char buffer
  char* new_string = malloc(newstr_len + 1);

  // Copy the memory at that pointer for the new string length
  memcpy(new_string, ctx->message + old_pointer, newstr_len);

  // Add a terminator. That's why we made + 1 sized
  new_string[newstr_len] = 0;

  return new_string;
}

char * parse_context_next_until_with_escapes(syslog_parse_context_t * ctx, char until_char, bool evaluate_escapes, bool or_eol) {
  bool escaped = false;

  size_t buf_inc = 48;
  size_t buf_len = buf_inc;

  char * buf = (char*) malloc(buf_len);
  char c = 0;
  int i = -1;
  while (!parse_context_is_eol(ctx)) {
    i++;
    if (i >= buf_len) {
      // We are about to set memory that has not been allocated.
      buf_len += buf_inc;
      buf = (char*) realloc(buf, buf_len);
    }

    parse_context_one(ctx, &c);

    if (escaped) {
      if (c == QUOTE || c == CLOSE_BRACKET || c == ESCAPE) {
        escaped = false;
      } else {
        if (evaluate_escapes) {
          // only those 3 characters support escape characters, so we are supposed
          // to treat it as unescaped and NOT throw away the ESCAPE character.
          // See https://tools.ietf.org/html/rfc5424#section-6.3
          buf[i] = ESCAPE;
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
      buf[i] = 0;
      return buf;
    }
    // Otherwise add the character
    buf[i] = c;
  }

  if (or_eol) {
    return buf;
  }

  free(buf);

  return NULL;
}

char** parse_context_get_structured_data_elements(syslog_parse_context_t * ctx, size_t * num_elements) {
  char start = 0;
  parse_context_peek(ctx, &start);

  size_t array_size = 12;
  size_t array_increment_size = 12;

  char ** datas = (char**) malloc(sizeof(char*) * (array_size));
  *num_elements = 0;

  if (start == 0) {
    // Structured data is nill
    parse_context_next_until(ctx, SEPARATOR, true);
    return datas;
  } else if (start != OPEN_BRACKET) {
    // Structured data must start with an open bracket, so this is invalid
    return NULL;
  }

	char pk = 0;
  while (parse_context_peek(ctx, &pk) && pk == OPEN_BRACKET) {
    if (*num_elements >= array_size) {
      // Need to realloc the array
      array_size += array_increment_size;
      datas = (char**) realloc(datas, sizeof(char*) * (array_size));
    }
    parse_context_one(ctx, &pk); // eat [
    // We need to find where structured data ends, but takes escapes into account
    datas[*num_elements] = parse_context_next_until_with_escapes(ctx, CLOSE_BRACKET, false, false);
    if (datas[*num_elements]) {
      *num_elements = *num_elements + 1;
    }
  }

  // If we are at EOL
  if (parse_context_is_eol(ctx)) {
    ctx->pointer++;
    return datas;
  }

  // If the last thing is a separator
  char buf = 0;
  parse_context_peek(ctx, &buf);
  if (buf == SEPARATOR) {
    ctx->pointer++;
    return datas;
  }

  // Uh oh. Need to free it cuz its bad
  free(datas);

  // Final character is something else which is bad
  return NULL;
}

syslog_parse_context_t create_parse_context(const char* raw_message) {
  // Pointer should automatically be initialized to 0
  syslog_parse_context_t ctx = { raw_message };
  return ctx;
}

int parse_structured_data_element(char* data_string, syslog_extended_property_t * property) {
  syslog_parse_context_t ctx = create_parse_context(data_string);

  // SD-ID
  property->id = parse_context_next_until_with_escapes(&ctx, SEPARATOR, true, true);
  if (!property->id) {
    return 0;
  }

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

  while (!parse_context_is_eol(&ctx)) {
    char* key = parse_context_next_until(&ctx, EQUALS, false);
    if (!key) {
      // Invalid because we need a key and value
      break;
    }

    // Value must start with a quote
    char quote = 0;
    if (!parse_context_one(&ctx, &quote) || quote != QUOTE) {
      // needs to be a quote
			free(key);
      break;
    }

    char* value = parse_context_next_until_with_escapes(&ctx, QUOTE, true, false);
    if (!value) {
      // No ending quote? What's wrong with you! courtsey of @dareed
      free(key);
      break;
    }

    num_elements++;

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

syslog_extended_property_t * get_structured_data(char** structured_data_elements, size_t num_elements) {
  // the message may contain multiple structured data parts, as in:
  // [exampleSDID@32473 iut="3" eventSource="Application" eventID="1011"][examplePriority@32473 class="high"]
  // in which case we are given each separately within the list array.
  syslog_extended_property_t * properties = (syslog_extended_property_t*) malloc(sizeof(syslog_extended_property_t) * 24 + 1);

  int ep_num = 0;

  for (size_t i = 0; i < num_elements; i++) {
    char* st_element = structured_data_elements[i];
    if (parse_structured_data_element(st_element, &properties[ep_num])) {
      ep_num++;
    }
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

bool parse_syslog(const char* raw_message, syslog_message_t * message) {
  syslog_parse_context_t ctx = create_parse_context(raw_message);

  // --- PRI
  char buf = 0;
  if (!parse_context_one(&ctx, &buf)) {
    return false;
  }

  if (buf != '<') {
    return false;
  }

  char* pri_string = parse_context_next_until(&ctx, '>', false);

  if (!pri_string) {
    return false;
  }

  int pri_value = atoi(pri_string);

  if (pri_value < 0 || pri_value > 191) {
    return false;
  }

  int facility_id = get_facility_id(pri_value);
  // Omg we got our first syslog value
  message->pri_value = pri_value;
  message->facility = facility_id / 8;
  message->severity = pri_value - facility_id;

	// We need to free this pri_string because it has been allocated
	// and isn't actually used
	free(pri_string);

  // --- VERSION
  message->syslog_version = parse_context_next_until(&ctx, SEPARATOR, false);
  if (!message->syslog_version || strlen(message->syslog_version) > 2) {
		// We do not need to deallocate the memory because none of the previous
		// stuff has been malloc'd if this returned null
		return false;
  }

  // --- TIMESTAMP
  char* timestamp = parse_context_next_until(&ctx, SEPARATOR, false);
  if (!timestamp) {
		free_syslog_message_t(message);
		return false;
  }

	if (strlen(timestamp) == 1 && timestamp[0] == 0) {
    // This means we want to get the current time
    time_t rawtime;
    time(&rawtime);

    message->timestamp = *localtime(&rawtime);
  } else {
    parse_iso_8601(timestamp, &message->timestamp);
  }

	// We do not need the timestamp anymore either
	free((char*) timestamp);

	// --- HOSTNAME
  char* hostname = parse_context_next_until(&ctx, SEPARATOR, false);
  if (!hostname) {
		free_syslog_message_t(message);
		return false;
  }
  message->hostname = filter_nil(hostname);

  // --- APP-NAME
  char* appname = parse_context_next_until(&ctx, SEPARATOR, false);
  if (!appname) {
		free_syslog_message_t(message);
		return false;
  }
  message->appname = filter_nil(appname);

  // --- PROCID
  // surprisingly, can be a string up to 128 chars
  char* process_id = parse_context_next_until(&ctx, SEPARATOR, false);
  if (!process_id) {
		free_syslog_message_t(message);
		return false;
  }
  message->process_id = filter_nil(process_id);

  // --- MSGID
  char* message_id = parse_context_next_until(&ctx, SEPARATOR, false);
  if (!message_id) {
		free_syslog_message_t(message);
		return false;
  }
  message->message_id = filter_nil(message_id);

  size_t num_structured_data;
  char ** structured_data = parse_context_get_structured_data_elements(&ctx, &num_structured_data);
  if (structured_data) {

	  if (num_structured_data < 1) {
	    message->structured_data = NULL;
	    message->structured_data_count = 0;
	  } else {
	    message->structured_data_count = num_structured_data;
	    message->structured_data = get_structured_data(structured_data, num_structured_data);
	  }

		// Need to free that char**
		for (size_t si = 0; si < num_structured_data; si++) {
			free(structured_data[si]);
		}

		// And the wrapper
		free(structured_data);

	}

  // --- MSG
  // Rest of the data is the message
  message->message = parse_context_is_eol(&ctx) ? NULL : parse_context_next_until(&ctx, '\0', true);

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
