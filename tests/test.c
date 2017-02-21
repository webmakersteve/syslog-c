# include "syslog.h"

int main(int argc, char** argv) {
	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";
	syslog_message_t msg = {};
	if (!parse_syslog(mm, &msg)) {
		return 1;
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
}
