/* See LICENSE file for copyright and license details. */

struct date {
	int day;
	int month;
	int year;
};

char * istask(const char * line, struct interval * in);
int getlabelid(const char * line);
int getdayid(const char * line, struct date *);
int gettime(const char * digit);
unsigned int now();
