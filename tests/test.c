#include "test.h"

void clar_start_clock() {
  float start_time = (float)clock()/CLOCKS_PER_SEC;
	int num_messages = 1000000;

	/*for (int i = 0; i < num_messages; i++) {
		syslog_message_t m = {};
		if (!parse_syslog_message_t(mm, &m)) {
			return;
		}
		free_syslog_message_t(&m);
	}

	/* Do work */

	float end_time = (float)clock()/CLOCKS_PER_SEC;

	float time_elapsed = end_time - start_time;

	printf("Took %f to process %d messages\n\n", time_elapsed, num_messages);
}
