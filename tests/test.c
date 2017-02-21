#include "syslog.h"
#include "time.h"

int main(int argc, char** argv) {
	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";
	// char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";

	syslog_message_t msg = {};
	if (!parse_syslog(mm, &msg)) {
		return 1;
	}

	float start_time = (float)clock()/CLOCKS_PER_SEC;
	int num_messages = 1000000;

	for (int i = 0; i < num_messages; i++) {
		syslog_message_t m = {};
		if (!parse_syslog(mm, &m)) {
			return 1;
		}
		free_syslog_message_t(&m);
	}

	/* Do work */

	float end_time = (float)clock()/CLOCKS_PER_SEC;

	float time_elapsed = end_time - start_time;

	printf("Took %f to process %d messages\n\n", time_elapsed, num_messages);

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
