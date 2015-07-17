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

struct interval {
	int start;
	int stop;
};

char * daynames[] = {
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};

struct labels {
	char * label;
	char mark;
};

struct labels convert[] = {
	{ "wd", 'w' },
	{ "L",  'L' },
	{ "k",  'k' },
	{ "p",  'p' },
	{ "!t", 't' },
	{ "!",  '-' },
	{ "!u", 'u' },
	{ "",   '+' },
	{ "ui", 'G' },
	{ "sp", 's' },
	{ "ph", 'P' },
	{ "le", '+' },
	{ "a",  'a' },
	/* UNKNOWN MUST be last */
	{ "UNKNOWN", '.' },
};

const int bin = 1 * 3600;
const int size = 24 * bin / 3600;
const int columns = 9;

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
istask(const char * line, struct interval * in) {
	char * ob; /* open bracket */
	if ((ob = strchr(line, '('))) {
		if ((in->start = gettime(ob + 1)) >= 0
				&& ob[6] == '-') {
			ob[0] = '\0';
			if ((in->stop = gettime(ob + 7)) >= 0 && ob[12] == ')') {
				return in->stop - in->start;
			} else if (ob[7] == ')') {
				time_t t;
				time(&t);
				struct tm * tm = localtime(&t);
				in->stop = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600;
				return in->stop - in->start;
			}
		}
	}
	return -1;
}

int
getlabelid(const char * line) {
	int i;
	/* -1 because the last one is UNKNOWN */
	for (i = 0; i < LENGTH(convert) - 1; i++) {
		if (!strcmp(convert[i].label, line))
			break;
	}
	return i;
}

int
getdayid(const char * line) {
	int a;
	if (sscanf(line, "%d.%d.%d -", &a, &a, &a) != 3)
		return -1;
	char * hyp = strchr(line, '-');
	if (!hyp)
		return -1;
	int i;
	for (i = 0; i < LENGTH(daynames); i++) {
		int ret;
		if (!(ret = strncmp(hyp + 2, daynames[i], strlen(daynames[i]))))
			break;
	}
	return (i < LENGTH(daynames)) ? i : -1;
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
		struct interval in;
		if ((time = istask(buf, &in)) >= 0) {
			int i = getlabelid(buf);
			printf("% 6d %2s %2d %c %d %d\n", time, buf, i, convert[i].mark, in.start / bin, in.stop / bin);
			continue;
		}
		if ((day = getdayid(buf)) >= 0)
			printf("%s", buf);
	}

	for (day = 0; day < LENGTH(daynames); day++)
		printf("%-9s %2d :\n", daynames[day], day);

	free(buf);
	return EXIT_SUCCESS;
}
