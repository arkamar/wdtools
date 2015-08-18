/* See LICENSE file for copyright and license details.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arg.h"
#include "config.h"
#include "utils.h"

#define LENGTH(X) (sizeof(X) / sizeof(X[0]))

static void freedata();
static int isnewmonth(const char * line);
static void initdata();
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
	unsigned int efc; /* working efficiency */
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
	fprintf(stderr, "usage: %s [-dmtw] [-ceM number] [-W label]\n", argv0);
	exit(1);
}

void
initdata() {
	data.year = calloc(LENGTH(convert), sizeof(int));
	if (!data.year)
		fprintf(stderr, "Error, cannot allocate memory\n");
	data.month = calloc(LENGTH(convert), sizeof(int));
	if (!data.month)
		fprintf(stderr, "Error, cannot allocate memory\n");
	data.week = calloc(LENGTH(convert), sizeof(int));
	if (!data.week)
		fprintf(stderr, "Error, cannot allocate memory\n");
	data.day = calloc(LENGTH(convert), sizeof(int));
	if (!data.day)
		fprintf(stderr, "Error, cannot allocate memory\n");
}

void
freedata() {
	free(data.year);
	free(data.month);
	free(data.week);
	free(data.day);
}

char *
sec2str(unsigned int sec) {
	static char buf[16];
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
		if (!strncasecmp(line, monthnames[i], strlen(monthnames[i])))
			return 1;

	return 0;
}

void
printlabels() {
	int i;
	printf("#   ");
	for (i = 1; i < columns; i++)
		printf("%6s ", convert[i].label);
	printf("%6s \n", "all");
}

void
printstatline(const char * label, const int * array, const float divisor) {
	int i;
	printf("%s ", label);
	for (i = 1; i < columns; i++)
		printf("%6.*f ", (array[i] / divisor < 1000) ? 2 : 1, array[i] / divisor);
	printf("%6.*f ", (array[0] / divisor < 1000) ? 2 : 1, array[0] / divisor);
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
	if (options.efc)
		printf("%s %.2f (%.2f)\n", label, time, time * 100.0 / options.efc);
	else
		printf("%s %.2f\n", label, time);
}

void
printtopay(const float time) {
	printpayline("--- To pay:", time);
	if (options.mph)
		printpayline("-M- To pay:", time * options.mph);
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
	int label, workingtime = 0;
	const unsigned int procrastination = getlabelid("!*");
	const unsigned int work = getlabelid("+");
	unsigned int payed = getlabelid("");

	ARGBEGIN {
	case 'c':
		columns = atoi(EARGF(usage()));
		if (columns > LENGTH(convert))
			columns = LENGTH(convert);
		break;
	case 'e':
		options.efc = atoi(EARGF(usage()));
		break;
	case 'd':
		options.flags |= F_DAY_STAT;
		break;
	case 'm':
		options.flags |= F_MONTH_STAT;
		break;
	case 'M':
		options.mph = atoi(EARGF(usage()));
		break;
	case 't':
		options.flags |= F_PRINT_TASK;
		break;
	case 'w':
		options.flags |= F_WEEK_STAT;
		break;
	case 'W':
		payed = getlabelid(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND;

	initdata();

	while (getline(&buf, &size, fp) > 0) {
		char * time;
		struct interval interval;
		if ((time = istask(buf, &interval))) {
			const int timeint = interval.stop - interval.start;
			label = getlabelid(buf);
			char * tmp = strchr(time, ')');
			if (!tmp)
				continue;
			tmp[0] = '\0';
			if (options.flags & F_PRINT_TASK) {
				printf("%-2s(%s", buf, time);
				if (tmp[-1] == '-')
					printf("%s", sec2str(interval.stop));
				printf(") %.2f:%s", (timeint) / 3600.0, tmp + 1);
			}
			set(timeint, label);
			set(timeint, 0);
			if (strchr(buf, '!'))
				set(timeint, procrastination);
			if (convert[label].mark == '+')
				set(timeint, work);
			if (label == payed)
				workingtime += timeint;
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
		if (strstr(buf, WEEK)) {
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
		if (!strncmp(buf, PAY_MARK, LENGTH(PAY_MARK))) {
			printtopay(workingtime / 3600.0);
			workingtime = 0;
		}
	}

	printdaystat();
	printf("\n");
	printweekstat();
	printf("\n");
	printmonthstat();
	printf("\n");
	printyearstat();
	printf("\n");
	printtopay(workingtime / 3600.0);

	freedata();
	free(buf);
	return EXIT_SUCCESS;
}
