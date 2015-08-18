#pragma once

#define LENGTH(X) (sizeof(X) / sizeof(X[0]))

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

struct labels {
	char * label;
	char mark;
};

static const struct labels convert[] = {
	{ "all",' ' },
	{ "",   '+' },
	{ "d",  '+' },
	{ "le", '+' },
	{ "wd", 'w' },
	{ "k",  'k' },
	{ "L",  'L' },
	{ "!t", 't' },
	{ "!",  '-' },
	{ "!u", 'u' },
	{ "!*", '-' },
	{ "+",  '+' },
	{ "ui", 'G' },
	{ "ja", '+' },
	{ "sp", 's' },
	{ "ph", 'P' },
	{ "a",  'a' },
	{ "p",  'p' },
	/* UNKNOWN MUST be the last one */
	{ "UNKN", '.' },
};

#define WEEK "WEEK"

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
