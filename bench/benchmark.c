
void test_syslog_message__benchmark(void) {
	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";

	int num_messages = 1140000;

	float start_time = (float)clock()/CLOCKS_PER_SEC;

	for (int i = 0; i < num_messages; i++) {
		syslog_message_t m = {};
		if (!parse_syslog_message_t(mm, &m)) {
			cl_fail("Could not parse the syslog message");
		}
		free_syslog_message_t(&m);
	}

	/* Do work */

	float end_time = (float)clock()/CLOCKS_PER_SEC;

	float time_elapsed = end_time - start_time;

	printf("\nTook %f to process %d messages without structured data\n\n", time_elapsed, num_messages);
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
