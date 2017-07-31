#include "syslog.h"
#include "time.h"

typedef struct bench_clock_t {
	float start_time;
	float end_time;
} bench_clock_t;

float get_current_time() {
	return (float) clock()/CLOCKS_PER_SEC;
}

void start_clock(bench_clock_t * bench_clock) {
	bench_clock->start_time = get_current_time();
}

void end_clock(bench_clock_t * bench_clock) {
	bench_clock->end_time = get_current_time();
}

float get_clock_elapsed_time(bench_clock_t * bench_clock) {
	float time_elapsed = bench_clock->end_time - bench_clock->start_time;
	if (time_elapsed < 0) {
		return 0.0;
	}
	return time_elapsed;
}

void reset_clock(bench_clock_t * bench_clock) {
	bench_clock->start_time = 0;
	bench_clock->end_time = 0;
}

void benchmark(int num_messages, bench_clock_t * bench_clock) {
	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";

	start_clock(bench_clock);

	int i;
	for (i = 0; i < num_messages; i++) {
		syslog_message_t m = {};
		if (!parse_syslog_message_t(mm, &m)) {
			printf("Failed to parse syslog message");
		} else {
			free_syslog_message_t(&m);
		}
	}

	/* Do work */

	end_clock(bench_clock);
}

void benchmark_with_structured_data(int num_messages, bench_clock_t * bench_clock) {
	char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID [exampleSDID@32473 eventSource=\"Application\" eventID=\"1011\"] Logging message...";

	start_clock(bench_clock);

	syslog_message_t m = {};

	int i;
	for (i = 0; i < num_messages; i++) {
		if (!parse_syslog_message_t(mm, &m)) {
			printf("Failed to parse syslog message");
		} else {
			free_syslog_message_t(&m);
		}
	}

	end_clock(bench_clock);
}

int main(int argc, char* argv[]) {
	bench_clock_t bench_clock = {};

	int num_messages = 12540000;

	benchmark(num_messages, &bench_clock);
	{
		float elapsed_time = get_clock_elapsed_time(&bench_clock);
		int mps = num_messages / elapsed_time;

		printf("Took %f to process %d messages without structured data, which is %d messages per second\n", elapsed_time, num_messages, mps);
	}
	reset_clock(&bench_clock);

	benchmark_with_structured_data(num_messages, &bench_clock);
	{
		float elapsed_time = get_clock_elapsed_time(&bench_clock);
		int mps = num_messages / elapsed_time;

		printf("Took %f to process %d messages with structured data, which is %d messages per second\n", elapsed_time, num_messages, mps);
	}
	reset_clock(&bench_clock);
}
