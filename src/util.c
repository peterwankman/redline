/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ermac.h"

#define MAXBUF		1024

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>

char get_key(int *status) {
	struct termios old, new = { 0 };
	char out;

	if(status == NULL) return 0;
	*status = RET_ERR_INTERNAL;

	tcgetattr(0, &old);

	new.c_lflag &= ~ICANON;
	new.c_lflag &= ~ECHO;
	new.c_cc[VMIN] = 1;
	new.c_cc[VTIME] = 0;

	if(tcsetattr(0, TCSANOW, &new) < 0) return '\0';
	if(read(0, &out, 1) < 0) return '\0';
	if(tcsetattr(0, TCSANOW, &old) < 0) return '\0';

	*status = RET_OK;
	return out;
}
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
char get_key(int *status) {
	DWORD cc;
	INPUT_RECORD irec;
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	KEY_EVENT_RECORD *event;

	if(status == NULL) return 0;

	if(h == NULL) return FALSE;

	for(;;) {
		ReadConsoleInput(h, &irec, 1, &cc);
		event = (KEY_EVENT_RECORD *)&irec.Event;

		if((irec.EventType == KEY_EVENT) &&
			(event->bKeyDown)) {
			*status = RET_OK;
			return irec.Event.KeyEvent.uChar.AsciiChar;
		}
	}
	return 0;
}
#endif

char *get_line(FILE *fp) {
	size_t len = MAXBUF;
	char buf[MAXBUF];
	char *end = NULL;
	char *ret = calloc(MAXBUF, 1);

	while(fgets(buf, MAXBUF, fp)) {
		if(len - strlen(ret) < MAXBUF)
			ret = realloc(ret, len *= 2);

		strcat(ret, buf);

		if((end = strrchr(ret, '\r')) != NULL) break;
		if((end = strrchr(ret, '\n')) != NULL) break;
	}
	if(end)
		*end = '\0';

	return ret;
}

void strtoupper(char *str) {
	size_t pos = 0;
	do {
		*str = toupper(*str);
	} while(*(++str));
}

char *str_alloc_copy(const char *str) {
	size_t str_size;
	char *out;

	if(str == NULL) return NULL;

	str_size = strlen(str) + 1;

	if((out = malloc(str_size)) == NULL) {
		fprintf(stderr, "edlin: String allocation failed.\n");
		return NULL;
	}

	memcpy(out, str, str_size);
	return out;
}

int is_integer(const char *str) {
	size_t len, pos;

	len = strlen(str);
	for(pos = 0; pos < len; pos++) {
		if(!isdigit(str[pos])) {
			if(pos != 0) return 0;
			if(str[pos] != '-') return 0;
		}
	}

	return 1;
}

int is_positive_integer(const char *str) {
	size_t len, pos;

	len = strlen(str);
	for(pos = 0; pos < len; pos++)
		if(!isdigit(str[pos]))
			return 0;

	return 1;
}

size_t num_len(int i) {
	size_t out = 0;
	while(i) {
		i /= 10;
		out++;
	}

	return out;
}

size_t estimate_lines(const size_t file_size) {
	return file_size / 80;
}

/* ctype character classification without the crashing */

#define def_ext_ctype(f) \
	int ext_ ## f(const int c) { \
		if(c < 0) return 0; \
		return f (c); \
	}

def_ext_ctype(isalnum);
def_ext_ctype(isalpha);
def_ext_ctype(iscntrl);
def_ext_ctype(isdigit);
def_ext_ctype(isgraph);
def_ext_ctype(islower);
def_ext_ctype(isprint);
def_ext_ctype(ispunct);
def_ext_ctype(isspace);
def_ext_ctype(isupper);
def_ext_ctype(isxdigit);

/**/

/* Grab 2 digits from somewhere in the string. */
static int get_number(const char *datestr, const size_t pos, const int max) {
	int a, b, out;

	if(pos >= strlen(datestr)) {
		fprintf(stderr, "get_number('%s'[%ld], %ld): Invalid position.\n",
			datestr, strlen(datestr), pos);
		return -1;
	}
	a = datestr[pos + 0];
	b = datestr[pos + 1];

	if(!(isdigit(a) && isdigit(b))) {
		fprintf(stderr, "get_number('%s', %ld): Invalid argument.\n",
			datestr, pos);
		return -1;
	}

	out = (a - '0') * 10 + b - '0';
	if(out > max) return -1;
	return out;
}
// [[CC]YY]MMDDhhmm[.ss]

static int month_length[13] = {
	0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int check_date(const int day, const int month, const int year) {
	int month_days;
	int leap_year = 0;

	/* Year 0 is a lie made up by flat earthers! */
	if((day < 1) || (month < 1) || (month > 12) || (year == 0))
		return -1;

	month_days = month_length[month];

	if(month == 2) {
		if((year % 4) != 0) {
			leap_year = 0;
		} else if((year % 100) != 0) {
			leap_year = 1;
		} else if((year % 400) != 0) {
			leap_year = 0;
		} else {
			leap_year = 1;
		}
	}
	
	if(day > month_days + leap_year) return -1;

	return 1;
}

struct tm parse_tm(const char *datestr, int *status) {
	int century = -1, year = -1, month = -1, day = -1;
	int hour = -1, minute = -1, second = -1;
	size_t datelen;
	struct tm out = { 0 };
	const char *dotpos, *seekpos = datestr;

	if((datestr == NULL) || (status == NULL)) {
		fprintf(stderr, "parse_date(): Invalid argument.\n");
		goto fail;
	}
	*status = 0;

	if((dotpos = strchr(datestr, '.')) == NULL) {
		second = 0;
		datelen = strlen(datestr);
	} else {
		/* Seconds can be over 59 because leap seconds exist. */
		if((second = get_number(dotpos, 1, 60)) == -1) {
			fprintf(stderr, "parse_date(): Invalid number of seconds.\n");
			goto fail;
		}
		datelen = dotpos - datestr;
	}

	if(datelen < 8) {
		fprintf(stderr, "parse_date(): Date string too short.");
		goto fail;
	}

	year = 0;
	switch(datelen) {
		case 12:
			if((century = get_number(seekpos, 0, 99)) == -1) {
				fprintf(stderr, "parse_date(): Invalid century.\n");
				goto fail;
			}
			seekpos += 2;

		case 10:
			if((year = get_number(seekpos, 0, 99)) == -1) {
				fprintf(stderr, "parse_date(): Invalid year.\n");
				goto fail;
			}
			seekpos += 2;

		case 8:
			month = get_number(seekpos, 0, 12);
			day = get_number(seekpos, 2, 31);
			hour = get_number(seekpos, 4, 23);
			minute = get_number(seekpos, 6, 59);
			break;
	}

	if(century == -1) {
		if(year < 69)
			century = 20;
		else
			century = 19;
	}
	year += century * 100;

	if(check_date(day, month, year) != 1) {
		fprintf(stderr, "parse_date(): Not a real date!\n");
		goto fail;
	}

	out.tm_year = year - 1900;
	out.tm_mon = month - 1;
	out.tm_mday = day;
	out.tm_hour = hour;
	out.tm_min = minute;
	out.tm_sec = second;

	*status = 1;
fail:
	return out;
}

time_t parse_time(const char *datestr, int *status) {
	struct tm stm;

	stm = parse_tm(datestr, status);
	if(status == 0)	return 0;

	return mktime(&stm);
}
