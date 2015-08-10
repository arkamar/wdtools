#include <time.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "utils.h"

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

char *
istask(const char * line, struct interval * in) {
	char * ob; /* open bracket */
	char * hp; /* hyphen */
	if ((ob = strchr(line, '('))) {
		if ((in->start = gettime(ob + 1)) >= 0
				&& (hp = strchr(ob, '-'))) {
			/* hack to speedup label search */
			ob[0] = '\0';
			if ((in->stop = gettime(hp + 1)) >= 0 && strchr(hp, ')')) {
				return ob + 1;
			} else if (hp[1] == ')') {
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
