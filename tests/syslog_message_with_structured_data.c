#include "test.h"

void test_syslog_message_with_structured_data__can_be_parsed(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";

	if (!parse_syslog_message_t(mm, &msg)) {
		cl_fail("Could not parse the syslog message");
	}

	cl_assert_equal_i(msg.severity, 5);
	cl_assert_equal_i(msg.facility, 20);
	cl_assert_equal_i(msg.pri_value, 165);
	cl_assert_equal_s(msg.syslog_version, "1");

	cl_assert_equal_s(msg.message_id, "MSGID");
	cl_assert_equal_s(msg.hostname, "hostname");
	cl_assert_equal_s(msg.appname, "appname");
	cl_assert_equal_s(msg.process_id, "PROCID");

	cl_assert_equal_s(msg.message, "Logging message...");

	char timestring[100];
	strftime(timestring, 100, "%x - %I:%M%p", &msg.timestamp);

	cl_assert_equal_s(timestring, "12/16/16 - 12:00PM");

	cl_assert_(msg.structured_data_count == 1, "Expected 1 structured data field");

	syslog_extended_property_t * property =
		(syslog_extended_property_t *) &msg.structured_data[0];

	cl_assert_equal_s(property->id, "exampleSDID@32473");

	cl_assert_(property->num_pairs == 2, "Expected 2 pairs");

	syslog_extended_property_value_t * pair =
		(syslog_extended_property_value_t *) &property->pairs[0];

	cl_assert_equal_s(pair->key, "eventSource");
	cl_assert_equal_s(pair->value, "Application");

	// Go to the next one
	pair++;

	cl_assert_equal_s(pair->key, "eventID");
	cl_assert_equal_s(pair->value, "1011");

	free_syslog_message_t(&msg);
}
