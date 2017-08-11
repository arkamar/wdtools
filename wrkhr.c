/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "config.h"
#include "utils.h"

static struct options {
	unsigned int mph; /* money per hour */
	unsigned int efc; /* working efficiency */
#define F_PRINT_TASK      0x0001
#define F_PRINT_DIFF      0x0002
#define F_PRINT_REAL      0x0004
#define F_PRINT_TIME      0x0010
#define F_PRINT_HEAD      0x0020
#define F_PRINT_LAST      0x0040
#define F_PRINT_LAST_DAY  0x0080
#define F_PRINT_EVERY_DAY 0x0100
	unsigned int flags;
} options;

struct string {
	long counter;
	size_t len;
	char * str;
};

struct vector {
	size_t capacity;
	size_t size;
	struct string * data;
};

char *argv0;
struct vector workinglabels;

static
void
usage(void) {
	fprintf(stderr, "usage: %s [-dDhlLprt] [-W LABEL] [-eM NUMBER]\n", argv0);
	exit(1);
}

static
void
init() {
	if (!(workinglabels.data = calloc(sizeof(struct string), 20))) {
		fprintf(stderr, "Cannot alloc enough memory\n");
		return;
	}
	workinglabels.capacity = 20;
}

static
int
resize() {
	if (workinglabels.size == workinglabels.capacity) {
		const size_t newcap = workinglabels.capacity * 3 / 2;
		void * newdata = realloc(workinglabels.data, newcap * sizeof(struct string));
		if (newdata) {
			workinglabels.data = newdata;
			workinglabels.capacity = newcap;
		} else {
			fprintf(stderr, "Cannot realloc memory\n");
			return -1;
		}
	}
	return 0;
}

static
void
freedata() {
	size_t i;
	for (i = 0; i < workinglabels.size; i++) {
		free(workinglabels.data[i].str);
	}
	free(workinglabels.data);
}

static
int
compare(const char * str, const long time) {
	size_t i;
	const size_t len = strlen(str);
	for (i = 0; i < workinglabels.size; i++) {
		struct string * item = &workinglabels.data[i];
		if (len != item->len)
			continue;
		if (!memcmp(item->str, str, len)) {
			item->counter += time;
			return 0;
		}
	}
	return 1;
}

static
void
add(const char * str, const long time) {
	if (resize())
		return;
	struct string * item = &workinglabels.data[workinglabels.size];
	item->len = strlen(str);
	item->str = strndup(str, item->len);
	item->counter = time;
	workinglabels.size++;
}

static
void
store(const char * str, const long time) {
	if (compare(str, time))
		add(str, time);
}

static
void
reset() {
	size_t i;
	for (i = 0; i < workinglabels.size; i++)
		workinglabels.data[i].counter = 0;
}

static
void
printbb() {
	int i, col = 0;
	if (options.flags & F_PRINT_DIFF && options.efc)
		col++;
	if (options.efc)
		col++;
	if (options.flags & F_PRINT_REAL)
		col++;
	printf("==========");
	for (i = 0; i < col; i++) {
		if (i)
			printf("==");
		if (options.flags & F_PRINT_TIME)
			printf("=======");
		if (options.mph)
			printf("=========");
	}
	printf("\n");
}

static
void
printsplitter() {
	int i, col = 0;
	if (options.flags & F_PRINT_DIFF && options.efc)
		col++;
	if (options.efc)
		col++;
	if (options.flags & F_PRINT_REAL)
		col++;
	printf("----------");
	for (i = 0; i < col; i++) {
		if (i)
			printf("-+");
		if (options.flags & F_PRINT_TIME)
			printf("-------");
		if (options.mph)
			printf("---------");
	}
	printf("\n");
}

static
void
printvalues(float value, int * iteration) {
	if (*iteration)
		printf(" |");
	if (options.flags & F_PRINT_TIME)
		printf(" %6.2f", value);
	if (options.mph)
		printf(" %8.1f", value * options.mph);
	++*iteration;
}

static
void
printline(const char * label, long time) {
	printf("%-10s", label);
	int iteration = 0;
	if (options.efc)
		printvalues(time / 3600.0 * 100.0 / options.efc, &iteration);
	if (options.flags & F_PRINT_REAL)
		printvalues(time / 3600.0, &iteration);
	if (options.flags & F_PRINT_DIFF && options.efc)
		printvalues(time / 3600.0 * (100.0 / options.efc - 1), &iteration);
	printf("\n");
}

static
void
printcolname(const char * name, int * i) {
	const int pt = (options.flags & F_PRINT_TIME) ? 1 : 0;
	const int pm = (options.mph) ? 1 : 0;
	const int space = 6 * pt + 8 * pm + (pm + pt - 1);
	int sb = (space + strlen(name)) / 2;
	sb += ((space + strlen(name)) % 2) ? 1 : 0;
	if (*i)
		printf(" |");
	const int sa = (space - strlen(name)) / 2;
	printf(" %*s%*s", sb, name, sa, "");
	++*i;
}

static
void
printhead() {
	int i = 0, cols = 0;
	printf("%-10s", "label");
	if (options.efc) {
		printcolname("optim", &i);
		cols++;
	}
	if (options.flags & F_PRINT_REAL) {
		printcolname("real", &i);
		cols++;
	}
	if (options.flags & F_PRINT_DIFF && options.efc) {
		printcolname("diff", &i);
		cols++;
	}
	printf("\n");
	printf("%-10s", "");
	for (i = 0; i < cols; i++) {
		if (i)
			printf(" |");
		if (options.flags & F_PRINT_TIME)
			printf(" %6s", "[h]");
		if (options.mph)
			printf(" %8s", "[ ]");
	}
	printf("\n");
}

static
void
print(long other) {
	size_t i, sum = 0;
	printbb();
	if (options.flags & F_PRINT_HEAD) {
		printhead();
		printsplitter();
	}
	for (i = 0; i < workinglabels.size; i++) {
		if (workinglabels.data[i].counter)
			printline(workinglabels.data[i].str, workinglabels.data[i].counter);
		sum += workinglabels.data[i].counter;
	}
	if (other)
		printline("other", other);
	if (sum + other) {
		printsplitter();
		printline("all", sum + other);
	}
	printbb();
}

static
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
	static size_t capacity = 0;
	int len;
	int label;
	long workingtime = 0;
	char * labelchar = "";
	char paymark[16];
	int  paymarklen = 0;
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
		labelchar = EARGF(usage());
		payed = getlabelid(labelchar);
		break;
	case 'd':
		options.flags |= F_PRINT_DIFF;
		break;
	case 'D':
		options.flags |= F_PRINT_EVERY_DAY;
		break;
	case 'r':
		options.flags |= F_PRINT_REAL;
		break;
	case 't':
		options.flags |= F_PRINT_TIME;
		break;
	case 'h':
		options.flags |= F_PRINT_HEAD;
		break;
	case 'l':
		options.flags |= F_PRINT_LAST;
		break;
	case 'L':
		options.flags |= F_PRINT_LAST_DAY;
		break;
	default:
		usage();
	} ARGEND;

	init();

	sprintf(paymark, PAY_MARK "%s", labelchar);
	paymarklen = strlen(paymark);

	while ((len = getline(&buf, &capacity, fp)) > 0) {
		char * time;
		struct interval interval;
		if ((time = istask(buf, &interval))) {
			const int timeint = interval.stop - interval.start;
			label = getlabelid(buf);
			if (label != payed)
				continue;
			char * tmp = strchr(time, ')');
			if (!tmp)
				continue;
			char * hyphen = strchr(tmp + 1, '-');
			char * wl;
			if (hyphen) {
				hyphen[-1] = '\0';
				wl = tmp + 2;
				store(wl, timeint);
			} else {
				workingtime += timeint;
			}
			tmp[0] = '\0';
			if (options.flags & F_PRINT_TASK) {
				printf("%-2s(%s", buf, time);
				if (tmp[-1] == '-')
					printf("%s", sec2str(interval.stop));
				if (hyphen) {
					printf(") %.2f:[%s]%s", (timeint) / 3600.0, tmp + 2, hyphen + 1);
				} else {
					printf(") %.2f:%s", (timeint) / 3600.0, tmp + 1);
				}
			}
			continue;
		}
		if (getdayid(buf) >= 0 && options.flags & (F_PRINT_LAST_DAY | F_PRINT_EVERY_DAY)) {
			if (options.flags & F_PRINT_EVERY_DAY) {
				print(workingtime);
				puts(buf);
			}
			reset();
			workingtime = 0;
			continue;
		}
		if (len - 1 == paymarklen && !strncmp(buf, paymark, paymarklen)) {
			if (!(options.flags & (F_PRINT_LAST | F_PRINT_LAST_DAY | F_PRINT_EVERY_DAY)))
				print(workingtime);
			reset();
			workingtime = 0;
		}
	}

	print(workingtime);

	free(buf);
	freedata();

	return EXIT_SUCCESS;
}
