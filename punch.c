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

#define IDX(day, bin, label)	((day) * arrsize * LENGTH(convert) \
		+ (bin) * LENGTH(convert) + (label))

typedef void (*fun)(const int, const int, const int);
static void initdata();
static unsigned int rounded(const unsigned int x, const unsigned int y);
static char * sec2str(unsigned int sec);
static void set(const int day, const struct interval * interval, const int label);
static void usage();
static void maketable(const int label, const fun f);

static int inmin  = 0;
static int insec  = 0;
static int inhour = 0;
static int bin = 3600;
static int arrsize;
static int from;
static int to;
static char printtime = 0;
static char printnumpart = 0;
static char printboth = 0;
static int columns = (80 - 14) / 6;

static unsigned short * data;
static unsigned short daycounter[LENGTH(daynames)];

char *argv0;

void
usage(void) {
	fprintf(stderr, "usage: %s [-bnt] [-chms number]\n", argv0);
	exit(1);
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

char *
sec2str(unsigned int sec) {
	static char buf[16];
	sprintf(buf, "%d:%02d", sec / 3600, sec % 3600 / 60);
	return buf;
}

void
set(const int day, const struct interval * interval, const int label) {
	int i, tmp;
	if ((interval->stop - interval->start) < 0)
		return;
	tmp = interval->start;
	for (i = interval->start / bin; i < interval->stop / bin; i++) {
		data[IDX(day, i, label)] += (i + 1) * bin - tmp;
		tmp = (i + 1) * bin;
	}
	data[IDX(day, interval->stop / bin, label)] += interval->stop - tmp;
}

unsigned int
rounded(const unsigned int x, const unsigned int y) {
	return x / y + ((x % y > y / 2) ? 1 : 0);
}

void
initcolumnsinterval() {
	from = now() / bin - 3;
	if (columns > arrsize)
		columns = arrsize;
	if (from < 0)
		from = 0;
	to = from + columns;
	if (to > arrsize) {
		to = arrsize;
		from = to - columns;
	}
}

void
marktable(const int day, const int ibin, const int unused) {
	int label, counter, k;
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

void
numtable(const int day, const int ibin, const int label) {
	const unsigned int time = data[IDX(day, ibin, label)];
	if (time)
		if (printtime)
			printf(" %5s", sec2str(time));
		else
			printf(" %5.1f", (float) time * 100.0 / bin);
	else
		printf("      ");
}

void
maketablehdr() {
	int ibin;
	printf("              ");
	for (ibin = from; ibin < to; ibin++)
		printf(" %5s", sec2str(ibin * bin));
}

void
maketable(const int label, const fun f) {
	int day, ibin;
	printf("\n");
	for (day = 0; day < LENGTH(daynames); day++) {
		printf("%-9s %2d :", daynames[day], daycounter[day]);
		for (ibin = from; ibin < to; ibin++) {
			f(day, ibin, label);
		}
		printf("\n");
	}
}

int
main(int argc, char *argv[]) {
	FILE *fp = stdin;
	static char *buf = NULL;
	static size_t size = 0;
	int day, label;

	ARGBEGIN {
	case 'b':
		printboth = 1;
		break;
	case 'c':
		columns = atoi(EARGF(usage()));
		break;
	case 'h':
		inhour = atoi(EARGF(usage()));
		break;
	case 'm':
		inmin = atoi(EARGF(usage()));
		break;
	case 'n':
		printnumpart = 1;
		break;
	case 's':
		insec = atoi(EARGF(usage()));
		break;
	case 't':
		printtime = 1;
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

	initcolumnsinterval();

	maketablehdr();
	if (printnumpart || printboth)
		maketable(label, numtable);
	if (!printnumpart || printboth)
		maketable(label, marktable);

	free(data);
	free(buf);
	return EXIT_SUCCESS;
}
