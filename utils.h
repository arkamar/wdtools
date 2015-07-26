#pragma once

char * istask(const char * line, struct interval * in);
int getlabelid(const char * line);
int getdayid(const char * line);
int gettime(const char * digit);
unsigned int now();
