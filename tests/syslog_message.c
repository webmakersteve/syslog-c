#include "test.h"

void test_syslog_message__returns_false_on_garbage(void) {
	syslog_message_t msg = {};

	cl_assert_(false == parse_syslog_message_t(NULL, &msg), "NULL should not parse");
	cl_assert_(false == parse_syslog_message_t("", &msg), "Empty string should not parse");
	cl_assert_(false == parse_syslog_message_t("stuff", &msg), "'stuff' should not parse");
	cl_assert_(false == parse_syslog_message_t("<abc>stuff", &msg), "'<abc>stuff' should not parse");
	cl_assert_(false == parse_syslog_message_t("<abc>1 stuff", &msg), "'<abc>1 stuff' should not parse");
	cl_assert_(false == parse_syslog_message_t("<stuff", &msg), "'<stuff' should not parse");
	cl_assert_(false == parse_syslog_message_t("\n", &msg), "New line should not parse");
	cl_assert_(false == parse_syslog_message_t("\t\t\t\t\t\t\t\t\r\n", &msg), "Tabs should not parse");

}

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

void test_syslog_message__can_be_parsed_with_a_different_message(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z adifferenthostname adifferentappname ID MSGID2 Logging message... but this one is kinda different";

	if (!parse_syslog_message_t(mm, &msg)) {
		cl_fail("Could not parse the syslog message");
	}

	cl_assert_equal_i(msg.severity, 5);
	cl_assert_equal_i(msg.facility, 20);
	cl_assert_equal_i(msg.pri_value, 165);
	cl_assert_equal_s(msg.syslog_version, "1");

	cl_assert_equal_s(msg.message_id, "MSGID2");
	cl_assert_equal_s(msg.hostname, "adifferenthostname");
	cl_assert_equal_s(msg.appname, "adifferentappname");
	cl_assert_equal_s(msg.process_id, "ID");

	cl_assert_equal_s(msg.message, "Logging message... but this one is kinda different");

	char timestring[100];
	strftime(timestring, 100, "%x - %I:%M%p", &msg.timestamp);

	cl_assert_equal_s(timestring, "12/16/16 - 12:00PM");

	cl_assert_(msg.structured_data == 0, "There should be no structured data");

	free_syslog_message_t(&msg);
}

void test_syslog_message__honors_null_fields(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 2016-12-16T12:00:00.000Z - - - - - Logging message...";

	if (!parse_syslog_message_t(mm, &msg)) {
		cl_fail("Could not parse the syslog message");
	}

	cl_assert_equal_i(msg.severity, 5);
	cl_assert_equal_i(msg.facility, 20);
	cl_assert_equal_i(msg.pri_value, 165);
	cl_assert_equal_s(msg.syslog_version, "1");

	cl_assert_equal_s(msg.hostname, "");
	cl_assert_equal_s(msg.appname, "");
	cl_assert_equal_s(msg.process_id, "");
	cl_assert_equal_s(msg.message_id, "");

	cl_assert_equal_s(msg.message, "Logging message...");

	cl_assert_(msg.structured_data == 0, "There should be no structured data");

	free_syslog_message_t(&msg);
}

void test_syslog_message__honors_null_timestamp(void) {
	syslog_message_t msg = {};

	char * mm = "<165>1 - hostname appname PROCID MSGID Logging message...";

	if (!parse_syslog_message_t(mm, &msg)) {
		cl_fail("Could not parse first syslog message");
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

	char compare[100];

	{
		// This means we want to get the current time
		time_t rawtime;
		time(&rawtime);

		struct tm now_tm = *localtime(&rawtime);

		strftime(compare, 100, "%x - %I:%M%p", &now_tm);
	}

	cl_assert_equal_s(timestring, compare);

	cl_assert_(msg.structured_data == 0, "There should be no structured data");

	free_syslog_message_t(&msg);

}

void test_syslog_message__parses_pri_values_correctly() {
	syslog_message_t msg = {};

	if (!parse_syslog_message_t("<160>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 0);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<161>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 1);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<162>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 2);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<163>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 3);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<164>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 4);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<165>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 5);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<166>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 6);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<167>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 7);
	cl_assert_equal_i(msg.facility, 20);

	if (!parse_syslog_message_t("<168>1 2016-12-16T12:00:00.000Z - - - - - Logging message...", &msg)) {
		cl_fail("Could not parse first syslog message");
	}

	cl_assert_equal_i(msg.severity, 0);
	cl_assert_equal_i(msg.facility, 21);

}
