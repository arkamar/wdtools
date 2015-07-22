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

static const char * daynames[] = {
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};

static const char * monthnames[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};

struct labels {
	char * label;
	char mark;
};

static const struct labels convert[] = {
	{ "all",' ' },
	{ "!*", '-' },
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

static void freedata();
static int getdayid(const char * line);
static int getlabelid(const char * line);
static int gettime(const char * digit);
static int isnewmonth(const char * line);
static char * istask(const char * line, struct interval * in);
static void initdata();
static unsigned int now();
static void printdaystat();
static void printweekstat();
static void printmonthstat();
static void printyearstat();
static void printstatline(const char * label, const int * array, const float divisor);
static void printpercstatline(const char * label, const int * array);
static void printlabels();
static char * sec2str(unsigned int sec);
static void set(const unsigned int time, const int label);
static void usage();

static unsigned int columns = 9;

static struct options {
	unsigned int mph; /* money per hour */
#define F_PRINT_TASK  0x01
#define F_DAY_STAT    0x02
#define F_WEEK_STAT   0x04
#define F_MONTH_STAT  0x08
	unsigned char flags;
} options;

static struct arrays {
	int * day;
	int * week;
	int * month;
	int * year;
} data;

char *argv0;

void
usage(void) {
	fprintf(stderr, "usage: %s [-dmtw] [-c number]\n", argv0);
	exit(1);
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
	data.year = calloc(LENGTH(convert), sizeof(int));
	if (!data.year)
		fprintf(stderr, "Shit, I cannot calloc\n");
	data.month = calloc(LENGTH(convert), sizeof(int));
	if (!data.month)
		fprintf(stderr, "Shit, I cannot calloc\n");
	data.week = calloc(LENGTH(convert), sizeof(int));
	if (!data.week)
		fprintf(stderr, "Shit, I cannot calloc\n");
	data.day = calloc(LENGTH(convert), sizeof(int));
	if (!data.day)
		fprintf(stderr, "Shit, I cannot calloc\n");
}

void
freedata() {
	free(data.year);
	free(data.month);
	free(data.week);
	free(data.day);
}

char *
istask(const char * line, struct interval * in) {
	char * ob; /* open bracket */
	if ((ob = strchr(line, '('))) {
		if ((in->start = gettime(ob + 1)) >= 0
				&& ob[6] == '-') {
			/* hack to speedup label search */
			ob[0] = '\0';
			if ((in->stop = gettime(ob + 7)) >= 0 && ob[12] == ')') {
				return ob + 1;
			} else if (ob[7] == ')') {
				in->stop = now();
				return ob + 1;
			}
		}
	}
	return NULL;
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

char *
sec2str(unsigned int sec) {
	static char buf[10];
	sprintf(buf, "%02d:%02d:%02d",
		sec / 3600,
		sec % 3600 / 60,
		sec % 3600 % 60);
	return buf;
}

int
isnewmonth(const char * line) {
	int i;
	for (i = 0; i < LENGTH(monthnames); i++)
		if (strstr(line, monthnames[i]))
			return 1;

	return 0;
}

void
printlabels() {
	int i;
	printf("#   ");
	for (i = 1; i < columns; i++)
		printf("%6s ", convert[i].label);
	printf("%6s\n", "all");
}

void
printstatline(const char * label, const int * array, const float divisor) {
	int i;
	printf("%s ", label);
	for (i = 1; i < columns; i++)
		printf("%6.2f ", array[i] / divisor);
	printf("%6.2f ", array[0] / divisor);
	printf("\n");
}

void
printpercstatline(const char * label, const int * array) {
	if (array[0])
		printstatline(label, array, array[0] / 100.0);
}

void
printyearstat() {
	printstatline("%YS", data.year, 3600.0);
	printpercstatline("%YP", data.year);
}

void
printmonthstat() {
	printstatline("%MS", data.month, 3600.0);
	printpercstatline("%MP", data.month);
}

void
printweekstat() {
	printstatline("%WS", data.week, 3600.0);
	printpercstatline("%WP", data.week);
}

void
printdaystat() {
	printlabels();
	printstatline("%DS", data.day, 3600.0);
	printpercstatline("%DP", data.day);
}

void
printpayline(const char * label, const float time) {
	printf("%s %.2f\n", label, time);
}

void
printtopay(const float time) {
	printpayline("--- To pay:", time);
}

void
set(const unsigned int time, const int label) {
	data.day[label] += time;
	data.week[label] += time;
	data.month[label] += time;
	data.year[label] += time;
}

int
main(int argc, char *argv[]) {
	FILE *fp = stdin;
	static char *buf = NULL;
	static size_t size = 0;
	int label;

	ARGBEGIN {
	case 'c':
		columns = atoi(EARGF(usage()));
		if (columns > LENGTH(convert))
			columns = LENGTH(convert);
		break;
	case 'd':
		options.flags |= F_DAY_STAT;
		break;
	case 'm':
		options.flags |= F_MONTH_STAT;
		break;
	case 't':
		options.flags |= F_PRINT_TASK;
		break;
	case 'w':
		options.flags |= F_WEEK_STAT;
		break;
	default:
		usage();
	} ARGEND;

	initdata();

	while (getline(&buf, &size, fp) > 0) {
		char * time;
		struct interval interval;
		if ((time = istask(buf, &interval))) {
			label = getlabelid(buf);
			char * tmp = strchr(time, ')');
			tmp[0] = '\0';
			if (options.flags & F_PRINT_TASK) {
				printf("%-2s(%s", buf, time);
				if (tmp[-1] == '-')
					printf("%s", sec2str(interval.stop));
				printf(") %.2f:%s", (interval.stop - interval.start) / 3600.0, tmp + 1);
			}
			set(interval.stop - interval.start, label);
			set(interval.stop - interval.start, 0);
			if (strchr(buf, '!'))
				set(interval.stop - interval.start, 1);
			continue;
		}
		int rd;
		if ((rd = getdayid(buf)) >= 0) {
			if (options.flags & F_DAY_STAT)
				printdaystat();
			if (options.flags & F_PRINT_TASK)
				printf("%s", buf);
			memset(data.day, 0, LENGTH(convert) * sizeof(int));
			continue;
		}
		if (strstr(buf, "WEEK")) {
			if (options.flags & F_WEEK_STAT)
				printweekstat();
			memset(data.week, 0, LENGTH(convert) * sizeof(int));
			continue;
		}
		if (isnewmonth(buf)) {
			if (options.flags & F_MONTH_STAT)
				printmonthstat();
			memset(data.month, 0, LENGTH(convert) * sizeof(int));
			continue;
		}
	}

	printdaystat();
	printf("\n");
	printweekstat();
	printf("\n");
	printmonthstat();
	printf("\n");
	printyearstat();
	printtopay(1.0);

	freedata();
	free(buf);
	return EXIT_SUCCESS;
}
