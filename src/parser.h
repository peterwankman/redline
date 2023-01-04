/*******************************************
 *  SPDX-License-Identifier: GPL-2.0-only  *
 * Copyright (C) 2022-2023  Martin Wolters *
 *******************************************/

#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>

#define EDPS_THIS_LINE	-1
#define EDPS_NO_LINE	-2

typedef enum edps_cmd_t {
	EDPS_CMD_APPEND,
	EDPS_CMD_ASK,
	EDPS_CMD_COPY,
	EDPS_CMD_DELETE,
	EDPS_CMD_EDIT,
	EDPS_CMD_END,
	EDPS_CMD_INSERT,
	EDPS_CMD_LIST,
	EDPS_CMD_MOVE,
	EDPS_CMD_PAGE,
	EDPS_CMD_QUIT,
	EDPS_CMD_REPLACE,
	EDPS_CMD_SEARCH,
	EDPS_CMD_TRANSFER,
	EDPS_CMD_WRITE,

	EDPS_CMD_NONE = -1
} edps_cmd_t;

typedef struct edps_instr_t {
	int start_line, end_line, only_line, target_line;
	edps_cmd_t command;
	uint32_t repeat;
	int ask;
	char *search_str, *replace_str;
	char *filename;
} edps_instr_t;

typedef struct edps_ctx_t edps_ctx_t;

void edps_free(edps_ctx_t *ctx);
edps_ctx_t *edps_new(const char *cmdline, const char *prompt, int *status);
int edps_parse(edps_ctx_t *ctx);
edps_instr_t *edps_get_instr(edps_ctx_t *ctx);

#endif
