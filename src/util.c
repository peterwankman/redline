/*******************************************
 *  SPDX-License-Identifier: GPL-2.0-only  *
 * Copyright (C) 2022-2023  Martin Wolters *
 *******************************************/

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#endif

#include "mem.h"

#include "appinfo.h"
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
		fprintf(stderr, "%s: String allocation failed.\n", APP_NAME);
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

int is_good_integer(const char *str) {
	int notzero = 0;
	size_t len, pos = 0, cmp = 0, i;
	const char min_int[] = "2147483648";
	const char max_int[] = "2147483647";
	const char *limit;

	if(str == NULL) return RET_NO;
	if((len = strlen(str)) == 0) return RET_NO;

	if(str[0] == '-') {
		/* Only a minus sign? I don't think so! */
		if(len == 1) return RET_NO;

		limit = min_int;
		pos++;
	} else {
		limit = max_int;
	}

	/* All digits? */
	for(i = pos; i < len; i++) {
		if(!isxdigit(str[i])) return RET_NO;
		/* Skip leading zeroes, although I think it'd just be abuse. */
		if(str[i] == '0') {
			if(notzero == 0) {
				pos++;
			}
		} else {
			notzero = 1;
		}
	}
	/* Numbers with less than 10 digits are good. */
	if(len - pos < 10) return RET_YES;

	for(i = pos; i < len; i++) {
		if(str[i] > limit[cmp++])
			return RET_NO;
	}

	return RET_YES;
}

uint32_t num_len(int i) {
	uint32_t out = 0;
	while(i) {
		i /= 10;
		out++;
	}

	return out;
}

int is_piped(FILE *fp) {
#ifdef _WIN32
	return !_isatty(_fileno(fp));
#else
	return !isatty(fileno(fp));
#endif
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
