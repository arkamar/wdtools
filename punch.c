/* See LICENSE file for copyright and license details.
 *
 * Purpose of this program is to colorize output of ii connected to
 * bitlbee. It also filters out join statuses.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"

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
istask(const char * line) {
	char * ob; /* open bracket */
	if ((ob = strchr(line, '('))) {
		if (istime(ob + 1)
				&& ob[6] == '-') {
			if (istime(ob + 7) && ob[12] == ')') {
				return 1;
			} else if (ob[7] == ')') {
				return 2;
			}
		}
	}
	return 0;
}

int
main(int argc, char *argv[]) {
	FILE *fp = stdin;
	static char *buf = NULL;
	static size_t size = 0;

	ARGBEGIN {
	default:
		usage();
	} ARGEND;

	while (getline(&buf, &size, fp) > 0) {
		if (istask(buf))
			printf("%s", buf);
	}

	free(buf);
	return EXIT_SUCCESS;
}
