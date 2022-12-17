/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifndef REPL_H_
#define REPL_H_

#include <stdint.h>
#include <stdio.h>

#include "dynarr.h"

#define DEFAULT_CURSOR		"*"
#define DEFAULT_PROMPT		"*"

typedef struct ed_doc_t {
	dynarr_t *lines_arr;
	uint32_t n_lines;
	char *filename;
} ed_doc_t;

void free_doc(ed_doc_t *doc);
int save_doc(ed_doc_t *doc, const char *filename, const uint32_t start_line, const uint32_t end_line);
ed_doc_t *load_doc(FILE *fp, const char *filename);
ed_doc_t *empty_doc(const char *filename);

int repl_main(FILE *input, ed_doc_t *ed_doc, const char *prompt, const char *cursor_marker);

#endif
