/* See LICENSE file for copyright and license details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "config.h"
#include "utils.h"

static char * sec2str(unsigned int sec);

static struct options {
	unsigned int mph; /* money per hour */
	unsigned int efc; /* working efficiency */
#define F_PRINT_TASK  0x01
#define F_DAY_STAT    0x02
#define F_WEEK_STAT   0x04
#define F_MONTH_STAT  0x08
	unsigned char flags;
} options;

char *argv0;

void
usage(void) {
	fprintf(stderr, "usage: %s \n", argv0);
	exit(1);
}

char *
sec2str(unsigned int sec) {
	static char buf[16];
	sprintf(buf, "%02d:%02d",
		sec / 3600,
		sec % 3600 / 60);
	return buf;
}

int
main(int argc, char *argv[]) {
	FILE *fp = stdin;
	static char *buf = NULL;
	static size_t size = 0;
	int label, workingtime = 0;
	unsigned int payed = getlabelid("");

	ARGBEGIN {
	case 'e':
		options.efc = atoi(EARGF(usage()));
		break;
	case 'M':
		options.mph = atoi(EARGF(usage()));
		break;
	case 'p':
		options.flags |= F_PRINT_TASK;
		break;
	case 'W':
		payed = getlabelid(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND;

	while (getline(&buf, &size, fp) > 0) {
		char * time;
		struct interval interval;
		if ((time = istask(buf, &interval))) {
			const int timeint = interval.stop - interval.start;
			label = getlabelid(buf);
			if (label != payed)
				continue;
			char * tmp = strchr(time, ')');
			tmp[0] = '\0';
			if (options.flags & F_PRINT_TASK) {
				printf("%-2s(%s", buf, time);
				if (tmp[-1] == '-')
					printf("%s", sec2str(interval.stop));
				printf(") %.2f:%s", (timeint) / 3600.0, tmp + 1);
			}
			continue;
		}
		if (!strncmp(buf, "+++", 3)) {
			printf("%s", buf);
			workingtime = 0;
		}
	}

	free(buf);

	return EXIT_SUCCESS;
}
