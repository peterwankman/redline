/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifndef LEXER_H_
#define LEXER_H_

typedef enum edlx_token_t {
	EDLX_TOKEN_INVALID = -1,
	EDLX_TOKEN_ERROR = -2,

	EDLX_TOKEN_NUMBER = 1,
	EDLX_TOKEN_TEXT,
	EDLX_TOKEN_STRING,
	EDLX_TOKEN_THIS_LINE,

	EDLX_TOKEN_DELIM_COMMA,
	EDLX_TOKEN_DELIM_SEMICOLON,

	/* ACDEILMPQRSTW */
	EDLX_TOKEN_KW_APPEND,
	EDLX_TOKEN_KW_COPY,
	EDLX_TOKEN_KW_DELETE,
	EDLX_TOKEN_KW_END,
	EDLX_TOKEN_KW_INSERT,
	EDLX_TOKEN_KW_LIST,
	EDLX_TOKEN_KW_MOVE,
	EDLX_TOKEN_KW_PAGE,
	EDLX_TOKEN_KW_QUIT,
	EDLX_TOKEN_KW_REPLACE,
	EDLX_TOKEN_KW_SEARCH,
	EDLX_TOKEN_KW_TRANSFER,
	EDLX_TOKEN_KW_WRITE,
	EDLX_TOKEN_KW_ASK,
	EDLX_TOKEN_KW_ASK_REPLACE,
	EDLX_TOKEN_KW_ASK_SEARCH,

	EDLX_TOKEN_EOL = 100,
	EDLX_TOKEN_EOF = 101
} edlx_token_t;

typedef struct edlx_ctx_t edlx_ctx_t;

void edlx_token_str(const edlx_token_t token);

edlx_ctx_t *edlx_ctx_new(const char *cmdline, int *status);
void edlx_ctx_free(edlx_ctx_t *ctx);
int edlx_step(edlx_ctx_t *ctx);
int edlx_rewind(edlx_ctx_t *ctx);

edlx_token_t edlx_get_token(edlx_ctx_t *ctx, int *status);
int edlx_get_required_token(edlx_ctx_t *ctx, edlx_token_t expect);
char *edlx_get_lexeme(edlx_ctx_t *ctx);
char edlx_get_lookahead(edlx_ctx_t *ctx, int *status);

void edlx_print_error(edlx_ctx_t *ctx, const char *errmsg, const int printline);

#endif
