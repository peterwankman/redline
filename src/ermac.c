/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <stdlib.h>

#include "leak.h"

#include "appinfo.h"
#include "ermac.h"
#include "util.h"

#define iswinderr(err_no) \
	((err_no == RET_ERR_MALLOC) || \
     (err_no == RET_ERR_OPEN) || \
     (err_no == RET_ERR_READ) || \
     (err_no == RET_ERR_WRITE))

static void print_windows_errmsg(const int winderr) {
#ifdef _WIN32
	char *winderrstr;

	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, winderr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&winderrstr, 0, NULL);

	fprintf(stderr, " -- %s", winderrstr);
	LocalFree(winderrstr);
#endif
}

char *str_error(const int err_no) {
	switch(err_no) {
		case RET_OK:			return str_alloc_copy("Ok");
		case RET_YES:			return str_alloc_copy("Yes");
		case RET_NO:			return str_alloc_copy("No");
		case RET_MORE:			return str_alloc_copy("More available");

		case RET_ERR_INTERNAL:	return str_alloc_copy("Internal error");
		case RET_ERR_NULLPO:	return str_alloc_copy("Illegal null pointer");
		case RET_ERR_MALLOC:	return str_alloc_copy("Memory allocation failed");
		case RET_ERR_SYNTAX:	return str_alloc_copy("Syntax error");
		case RET_ERR_NOTFOUND:	return str_alloc_copy("Not found");
		case RET_ERR_INVALID:	return str_alloc_copy("Invalid input");
		case RET_ERR_RANGE:		return str_alloc_copy("Invalid range");
		case RET_ERR_OPEN:		return str_alloc_copy("Open failed");
		case RET_ERR_READ:		return str_alloc_copy("Read error");
		case RET_ERR_WRITE:		return str_alloc_copy("Write error");
		case RET_ERR_NOWRITE:	return str_alloc_copy("Write protected");
		case RET_ERR_LEXER:		return str_alloc_copy("Internal lexer error");
		case RET_ERR_PARSER:	return str_alloc_copy("Internal parser error");
		case RET_ERR_DOUBLE:	return str_alloc_copy("Double fault");
	}

	return str_alloc_copy("Unknown error code");
}

int print_error(const int err_no) {
	char *errstr = str_error(err_no);

	fprintf(stderr, "%s: ", APP_NAME);

	if(errstr) {
		fprintf(stderr, "%s.\n", errstr);
		free(errstr);
#ifdef _WIN32
		if(iswinderr(err_no)) {
			print_windows_errmsg(GetLastError());
		}
#endif
	} else {
		fprintf(stderr, "Double fault. str_error() failed.\n");
		return RET_ERR_DOUBLE;
	}

	return err_no;
}
