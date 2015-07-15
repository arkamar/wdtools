/* See LICENSE file for copyright and license details.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arg.h"

#define LENGTH(X) (sizeof(X) / sizeof(X[0]))

char * daynames[] = {
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};

char *argv0;

static void
usage(void) {
	fprintf(stderr, "usage: %s [-c] [-n nick]\n", argv0);
	exit(1);
}

int
ishour(const char * digit) {
	return ((digit[0] == ' '
		|| (digit[0] >= '0' && digit[0] <= '2'))
		&& isdigit(digit[1]));
}

int
isminut(const char * digit) {
	return (digit[0] >= '0' && digit[0] <= '5' && isdigit(digit[1]));
}

int
istime(const char * time) {
	return (ishour(time) && time[2] == ':' && isminut(time + 3));
}

int
gettime(const char * digit) {
	int hour, min, sec;
	hour = min = sec = 0;
	switch (sscanf(digit, "%2d:%2d:%2d", &hour, &min, &sec)) {
	case 1:
		return hour * 3600;
	case 2:
		return hour * 3600 + min * 60;
	case 3:
		return hour * 3600 + min * 60 + sec;
	default:
		break;
	}
	return -1;
}

int
istask(const char * line) {
	char * ob; /* open bracket */
	int start, stop;
	if ((ob = strchr(line, '('))) {
		if ((start = gettime(ob + 1)) >= 0
				&& ob[6] == '-') {
			if ((stop = gettime(ob + 7)) >= 0 && ob[12] == ')') {
				return stop - start;
			} else if (ob[7] == ')') {
				time_t t;
				time(&t);
				struct tm * tm = localtime(&t);
				return tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 - start;
			}
		}
	}
	return -1;
}

int
main(int argc, char *argv[]) {
	FILE *fp = stdin;
	static char *buf = NULL;
	static size_t size = 0;
	int day;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	while (getline(&buf, &size, fp) > 0) {
		int time;
		if ((time = istask(buf)) >= 0)
			printf("% 6d %s", time, buf);
	}

	for (day = 0; day < LENGTH(daynames); day++)
		printf("%-9s %2d :\n", daynames[day], day);

	free(buf);
	return EXIT_SUCCESS;
}
