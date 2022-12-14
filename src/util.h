/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <time.h>

char get_key(int *status);
char *get_line(FILE *fp);
void strtoupper(char *str);
char *str_alloc_copy(const char *str);

int is_integer(const char *str);
int is_positive_integer(const char *str);
size_t num_len(int i);

#define dec_ext_ctype(f) \
	int ext_ ## f(const int c);

dec_ext_ctype(isalnum);
dec_ext_ctype(isalpha);
dec_ext_ctype(iscntrl);
dec_ext_ctype(isdigit);
dec_ext_ctype(isgraph);
dec_ext_ctype(islower);
dec_ext_ctype(isprint);
dec_ext_ctype(ispunct);
dec_ext_ctype(isspace);
dec_ext_ctype(isupper);
dec_ext_ctype(isxdigit);

struct tm parse_tm(const char *datestr, int *status);
time_t parse_time(const char *datestr, int *status);

#endif
