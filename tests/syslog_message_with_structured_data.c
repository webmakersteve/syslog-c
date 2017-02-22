#include "test.h"

void test_syslog_message_with_structured_data__can_be_parsed(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";

	if (!parse_syslog_message_t(mm, &msg)) {
		return;
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

	cl_assert_equal(msg.structured_data == 2, "There was structured data parsed");

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
