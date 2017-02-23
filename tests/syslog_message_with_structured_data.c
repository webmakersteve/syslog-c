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

void test_syslog_message_with_structured_data__log(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";

	if (!parse_syslog_message_t(mm, &msg)) {
		cl_fail("Could not parse the syslog message");
	}

	printf("Severity: %d\n", msg.severity);
	printf("Facility: %d\n", msg.facility);
	printf("Pri: %d\n", msg.pri_value);
	printf("ID: %s\n", msg.message_id);
	printf("Hostname: %s\n", msg.hostname);
	printf("Appname: %s\n", msg.appname);
	printf("PID: %s\n", msg.process_id);
	printf("Message: %s\n", msg.message);

	printf("Num of structured data elements: %lu\n", msg.structured_data_count);

	for (size_t i = 0; i < msg.structured_data_count; i++) {
		syslog_extended_property_t * property =
			(syslog_extended_property_t *) &msg.structured_data[i];

		printf("Property %lu. Key: %s\n", i, property->id);

		// Iterate over that now
		if (property->num_pairs > 0) {
			// Okay... we have some pairs
			for (size_t ii = 0; ii < property->num_pairs; ii++) {
				syslog_extended_property_value_t * pair =
					(syslog_extended_property_value_t *) &property->pairs[ii];

				printf("%s => %s\n", pair->key, pair->value);
			}
		}
	}

	free_syslog_message_t(&msg);
}

void test_syslog_message_with_structured_data__benchmark(void) {
	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";

	int num_messages = 1140000;

	float start_time = (float)clock()/CLOCKS_PER_SEC;

	for (int i = 0; i < num_messages; i++) {
		syslog_message_t m = {};
		if (!parse_syslog_message_t(mm, &m)) {
			cl_fail("Should not be able to parse this syslog message");
		}
		free_syslog_message_t(&m);
	}

	/* Do work */

	float end_time = (float)clock()/CLOCKS_PER_SEC;

	float time_elapsed = end_time - start_time;

	printf("Took %f to process %d messages with structured data\n\n", time_elapsed, num_messages);
}
