#include "test.h"

void test_syslog_message__can_be_parsed(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";

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

	cl_assert_(msg.structured_data == 0, "There should be no structured data");

	free_syslog_message_t(&msg);
}

void test_syslog_message__can_fail(void) {
	syslog_message_t msg = {};

	char * mm = "s<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";

	if (parse_syslog_message_t(mm, &msg)) {
		cl_fail("Should not be able to parse this syslog message");
	}

}

void test_syslog_message__log(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";

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

	free_syslog_message_t(&msg);
}

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
