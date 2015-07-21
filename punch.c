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
#define IDX(day, bin, label)	((day) * arrsize * LENGTH(convert) \
		+ (bin) * LENGTH(convert) + (label))

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
	/* UNKNOWN MUST be the last one */
	{ "UNKNOWN", '.' },
};

int inmin  = 0;
int insec  = 0;
int inhour = 0;
static int bin = 3600;
static int arrsize;
static int columns = (80 - 14) / 6;

static unsigned short * data;
static unsigned short daycounter[LENGTH(daynames)];


char *argv0;

static void
usage(void) {
	fprintf(stderr, "usage: %s [-chms number]\n", argv0);
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

unsigned int
now() {
	time_t t;
	time(&t);
	struct tm * tm = localtime(&t);
	return tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600;
}

void
initdata() {
	if (insec || inmin || inhour)
		bin = insec + inmin * 60 + inhour * 3600;
	arrsize = 24 * 3600 / bin;
	data = calloc(arrsize * LENGTH(convert) * LENGTH(daynames),
		sizeof(unsigned short));
	if (!data)
		fprintf(stderr, "Shit, I cannot calloc\n");
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
				in->stop = now();
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

void
set(const int day, const struct interval * interval, const int label) {
	int i, tmp;
	tmp = interval->start;
	for (i = interval->start / bin; i < interval->stop / bin; i++) {
		data[IDX(day, i, label)] += (i + 1) * bin - tmp;
		tmp = (i + 1) * bin;
	}
	data[IDX(day, interval->stop / bin, label)] += interval->stop - tmp;
}

unsigned short
get(const int day, const int ibin, const int label) {
	return data[IDX(day, ibin, label)];
}

unsigned int rounded(const unsigned int x, const unsigned int y) {
	return x / y + ((x % y > y / 2) ? 1 : 0);
}

char *
sec2str(unsigned int sec) {
	static char str[16];
	sprintf(str, "%d:%02d", sec / 3600, sec % 3600 / 60);
	return str;
}

int
main(int argc, char *argv[]) {
	FILE *fp = stdin;
	static char *buf = NULL;
	static size_t size = 0;
	int day, label;

	ARGBEGIN {
	case 'c':
		columns = atoi(EARGF(usage()));
		break;
	case 'h':
		inhour = atoi(EARGF(usage()));
		break;
	case 'm':
		inmin = atoi(EARGF(usage()));
		break;
	case 's':
		insec = atoi(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND;

	initdata();

	while (getline(&buf, &size, fp) > 0) {
		int time;
		struct interval interval;
		if ((time = istask(buf, &interval)) >= 0) {
			label = getlabelid(buf);
			set(day, &interval, label);
			continue;
		}
		int rd;
		if ((rd = getdayid(buf)) >= 0) {
			day = rd;
			memset(data + day * arrsize * LENGTH(convert), 0, arrsize * LENGTH(convert) * sizeof(unsigned short));
			daycounter[day] = 1;
		}
	}

	int from = now() / bin - 3;
	int to = from + columns;
	if (to > arrsize) {
		to = arrsize;
		from = to - columns;
		if (from < 0)
			from = 0;
	}
	int k, counter, ibin;
	printf("              ");
	for (ibin = from; ibin < to; ibin++)
		printf(" %5s", sec2str(ibin * bin));
	printf("\n");
	for (day = 0; day < LENGTH(daynames); day++) {
		printf("%-9s %2d :", daynames[day], daycounter[day]);
		for (ibin = from; ibin < to; ibin++) {
			const unsigned int time = data[IDX(day, ibin, label)];
			if (time)
				printf(" %5s", sec2str(time));
			else
				printf("      ");
		}
		printf("\n");
	}
	printf("\n");
	for (day = 0; day < LENGTH(daynames); day++) {
		printf("%-9s %2d :", daynames[day], daycounter[day]);
		for (ibin = from; ibin < to; ibin++) {
			printf(" ");
			for (counter = 0, label = 0; label < LENGTH(convert); label++) {
				const unsigned int time = rounded(data[IDX(day, ibin, label)] * 5,
					bin * daycounter[day]);
				for (k = 0; k < time; k++) {
					if (counter < 5)
						printf("%c", convert[label].mark);
					counter++;
				}
			}
			for (; counter < 5; counter++)
				printf(" ");
		}
		printf("\n");
	}

	free(data);
	free(buf);
	return EXIT_SUCCESS;
}
