/*******************************************
 *  SPDX-License-Identifier: GPL-2.0-only  *
 * Copyright (C) 2022-2023  Martin Wolters *
 *******************************************/

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "ermac.h"
#include "lexer.h"
#include "util.h"

#define PREALLOC_LEXEME 16

typedef enum edlx_state_t {
	EDLX_STATE_START,
	EDLX_STATE_NUMBER,
	EDLX_STATE_COMMAND,
	EDLX_STATE_TEXT,
	EDLX_STATE_DELIM,
	EDLX_STATE_THIS_LINE,
	EDLX_STATE_ASK,
	EDLX_STATE_ASK_REPLACE,
	EDLX_STATE_ASK_SEARCH,
	EDLX_STATE_STRING,
	EDLX_STATE_STRING_ESCAPE,
	EDLX_STATE_STRING_END,
	EDLX_STATE_EOL,
	EDLX_STATE_INVALID
} edlx_state_t;

typedef struct edlx_keyword_table_t {
	const char *keyword;
	const edlx_token_t token;
} edlx_keyword_table_t;

static const edlx_keyword_table_t edlx_keyword_table[] = {
	{ "A", EDLX_TOKEN_KW_APPEND },
	{ "a", EDLX_TOKEN_KW_APPEND },
	{ "C", EDLX_TOKEN_KW_COPY },
	{ "c", EDLX_TOKEN_KW_COPY },
	{ "D", EDLX_TOKEN_KW_DELETE },
	{ "d", EDLX_TOKEN_KW_DELETE },
	{ "E", EDLX_TOKEN_KW_END },
	{ "e", EDLX_TOKEN_KW_END },
	{ "I", EDLX_TOKEN_KW_INSERT },
	{ "i", EDLX_TOKEN_KW_INSERT },
	{ "L", EDLX_TOKEN_KW_LIST },
	{ "l", EDLX_TOKEN_KW_LIST },
	{ "M", EDLX_TOKEN_KW_MOVE },
	{ "m", EDLX_TOKEN_KW_MOVE },
	{ "P", EDLX_TOKEN_KW_PAGE },
	{ "p", EDLX_TOKEN_KW_PAGE },
	{ "Q", EDLX_TOKEN_KW_QUIT },
	{ "q", EDLX_TOKEN_KW_QUIT },
	{ "R", EDLX_TOKEN_KW_REPLACE },
	{ "r", EDLX_TOKEN_KW_REPLACE },
	{ "S", EDLX_TOKEN_KW_SEARCH },
	{ "s", EDLX_TOKEN_KW_SEARCH },
	{ "T", EDLX_TOKEN_KW_TRANSFER },
	{ "t", EDLX_TOKEN_KW_TRANSFER },
	{ "W", EDLX_TOKEN_KW_WRITE },
	{ "w", EDLX_TOKEN_KW_WRITE },
	{ "?R", EDLX_TOKEN_KW_ASK_REPLACE },
	{ "?r", EDLX_TOKEN_KW_ASK_REPLACE },
	{ "?S", EDLX_TOKEN_KW_ASK_SEARCH },
	{ "?s", EDLX_TOKEN_KW_ASK_SEARCH }
};

void edlx_token_str(const edlx_token_t token) {
	switch(token) {
		case EDLX_TOKEN_NUMBER:				printf("NUMBER");		break;
		case EDLX_TOKEN_DELIM_COMMA:		printf("COMMA");		break;
		case EDLX_TOKEN_DELIM_SEMICOLON:	printf("SEMICOLON");	break;
		case EDLX_TOKEN_TEXT:				printf("TEXT");			break;
		case EDLX_TOKEN_STRING:				printf("STRING");		break;
		case EDLX_TOKEN_THIS_LINE:			printf("THIS");			break;
		case EDLX_TOKEN_KW_APPEND:			printf("KW_APPEND");	break;
		case EDLX_TOKEN_KW_COPY:			printf("KW_COPY");		break;
		case EDLX_TOKEN_KW_DELETE:			printf("KW_DELETE");	break;
		case EDLX_TOKEN_KW_END:				printf("KW_END");		break;
		case EDLX_TOKEN_KW_INSERT:			printf("KW_INSERT");	break;
		case EDLX_TOKEN_KW_LIST:			printf("KW_LIST");		break;
		case EDLX_TOKEN_KW_MOVE:			printf("KW_MOVE");		break;
		case EDLX_TOKEN_KW_PAGE:			printf("KW_PAGE");		break;
		case EDLX_TOKEN_KW_QUIT:			printf("KW_QUIT");		break;
		case EDLX_TOKEN_KW_REPLACE:			printf("KW_REPLACE");	break;
		case EDLX_TOKEN_KW_SEARCH:			printf("KW_SEARCH");	break;
		case EDLX_TOKEN_KW_TRANSFER:		printf("KW_TRANSFER");	break;
		case EDLX_TOKEN_KW_WRITE:			printf("KW_WRITE");		break;
		case EDLX_TOKEN_KW_ASK:				printf("KW_ASK");		break;
		case EDLX_TOKEN_KW_ASK_REPLACE:		printf("KW_REPLACE?");	break;
		case EDLX_TOKEN_KW_ASK_SEARCH:		printf("KE_SEARCH?");	break;
		case EDLX_TOKEN_EOL:				printf("END OF LINE");	break;
		case EDLX_TOKEN_EOF:				printf("END OF FILE");	break;
		case EDLX_TOKEN_INVALID:			printf("INVALID");		break;
		case EDLX_TOKEN_ERROR:				printf("ERROR");		break;
		default:							printf("UNKNOWN");		break;
	}
}

static edlx_token_t get_delim(const char *lexeme) {
	switch(lexeme[0]) {
		case ',':	return EDLX_TOKEN_DELIM_COMMA;
		case ';':	return EDLX_TOKEN_DELIM_SEMICOLON;
	}
	return EDLX_TOKEN_INVALID;
}

static edlx_token_t get_command(const char *lexeme) {
	size_t pos, n_tokens;

	n_tokens = sizeof(edlx_keyword_table) / sizeof(edlx_keyword_table_t);
	for(pos = 0; pos < n_tokens; pos++)
		if(!strcmp(lexeme, edlx_keyword_table[pos].keyword))
			return edlx_keyword_table[pos].token;

	return EDLX_TOKEN_INVALID;
}

struct edlx_ctx_t {
	const char *cmdline;

	char *curr_lexeme;
	size_t curr_lexeme_alloced, curr_lexeme_size;
	size_t curr_lexeme_start, curr_lexeme_end;

	edlx_token_t curr_token;

	struct edlx_ctx_t *last_state;
};

/**/

static int isdelim(const char c) {
	size_t i;
	static const char delim[] = ",;";

	for(i = 0; i < sizeof(delim); i++)
		if(c == delim[i]) return 1;
	return 0;
}

static int iscmd(const char c) {
	size_t i;
	static const char cmd[] = "ACDEILMPQRSTW";

	for(i = 0; i < sizeof(cmd); i++)
		if(toupper(c) == cmd[i]) return 1;
	return 0;
}

/**/

void edlx_ctx_free(edlx_ctx_t *ctx) {
	if(ctx == NULL) return;

	if(ctx->last_state != NULL)
		edlx_ctx_free(ctx->last_state);

	free(ctx->curr_lexeme);
	free(ctx);
}

edlx_ctx_t *edlx_ctx_new(const char *cmdline, int *status) {
	edlx_ctx_t *out;

	if(status == NULL) return NULL;

	*status = RET_ERR_MALLOC;
	if((out = malloc(sizeof(edlx_ctx_t))) == NULL) return NULL;

	out->cmdline = cmdline;

	out->curr_lexeme = NULL;
	out->curr_lexeme_alloced = 0;
	out->curr_lexeme_size = 0;
	out->curr_lexeme_start = 0;
	out->curr_lexeme_end = 0;

	out->curr_token = EDLX_TOKEN_INVALID;
	out->last_state = NULL;

	*status = RET_OK;
	return out;
}

static char get_char(edlx_ctx_t *ctx) {
	char ret;

	if(ctx == NULL) return '\0';
	if(ctx->cmdline == NULL) return '\0';

	if(ctx->curr_lexeme_end >= strlen(ctx->cmdline))
		ret = '\0';
	else
		ret = ctx->cmdline[ctx->curr_lexeme_end];

	ctx->cmdline[ctx->curr_lexeme_end++];

	return ret;
}

static int extend_lexeme(edlx_ctx_t *ctx, const char newchar) {
	char *newbuf;

	if(ctx->curr_lexeme_size + 1 >= ctx->curr_lexeme_alloced) {
		if((newbuf = malloc(ctx->curr_lexeme_alloced + PREALLOC_LEXEME)) == NULL)
			return RET_ERR_MALLOC;
		memcpy(newbuf, ctx->curr_lexeme, ctx->curr_lexeme_alloced);
		free(ctx->curr_lexeme);

		ctx->curr_lexeme = newbuf;
		ctx->curr_lexeme_alloced += PREALLOC_LEXEME;
	}
	ctx->curr_lexeme[ctx->curr_lexeme_size++] = newchar;
	ctx->curr_lexeme[ctx->curr_lexeme_size] = '\0';
	return RET_OK;
}

/**/

static void prepare_ctx(edlx_ctx_t *ctx) {
	edlx_ctx_t *last_state;

	if(ctx->last_state != NULL)
		edlx_ctx_free(ctx->last_state);

	/* Save current state to enable rewinding. */
	if((ctx->last_state = malloc(sizeof(edlx_ctx_t))) == NULL) {
		fprintf(stderr, "edlin: Couldn't save lexer state.\n");
	} else {
		last_state = ctx->last_state;

		if((last_state->curr_lexeme = malloc(ctx->curr_lexeme_alloced)) == NULL) {
			fprintf(stderr, "edlin: Couldn't save previous lexeme.\n");
			goto copy_failed;
		}

		memcpy(last_state->curr_lexeme, ctx->curr_lexeme, ctx->curr_lexeme_alloced);

		last_state->cmdline = ctx->cmdline;
		last_state->curr_lexeme_alloced = ctx->curr_lexeme_alloced;
		last_state->curr_lexeme_size = ctx->curr_lexeme_size;
		last_state->curr_lexeme_start = ctx->curr_lexeme_start;
		last_state->curr_lexeme_end = ctx->curr_lexeme_end;
		last_state->curr_token = ctx->curr_token;

		last_state->last_state = NULL;
	}

copy_failed:
	free(ctx->curr_lexeme);

	ctx->curr_lexeme = NULL;
	ctx->curr_lexeme_alloced = 0;
	ctx->curr_lexeme_size = 0;
	ctx->curr_lexeme_start = ctx->curr_lexeme_end;
}

int edlx_step(edlx_ctx_t *ctx) {
	int done = 0, skip = 0, addcurrchar = 0;
	char curr_char = 0;
	edlx_state_t state;
	edlx_token_t token;
	int status;

	if(ctx == NULL) return RET_ERR_NULLPO;

	prepare_ctx(ctx);
	state = EDLX_STATE_START;
	while(done == 0) {
		curr_char = get_char(ctx);

		addcurrchar = 1;

		switch(state) {
			case EDLX_STATE_START:
				if(curr_char == '\0') {
					state = EDLX_STATE_EOL;
					addcurrchar = 0;
					done = 1;
				} else if(ext_isspace(curr_char)) {
					ctx->curr_lexeme_start++;
					addcurrchar = 0;
				} else if(ext_isdigit(curr_char)) {
					state = EDLX_STATE_NUMBER;
				} else if(curr_char == '?') {
					state = EDLX_STATE_ASK;
				} else if(curr_char == '.') {
					state = EDLX_STATE_THIS_LINE;
				} else if(iscmd(curr_char)) {
					state = EDLX_STATE_COMMAND;
				} else if(ext_isalpha(curr_char)) {
					state = EDLX_STATE_TEXT;
				} else if(isdelim(curr_char)) {
					state = EDLX_STATE_DELIM;
				} else if(curr_char == '"') {
					addcurrchar = 0;
					state = EDLX_STATE_STRING;
				} else {
					state = EDLX_STATE_INVALID;
					addcurrchar = 0;
					skip = 1;
					done = 1;
				}
				break;

			case EDLX_STATE_NUMBER:
				if(ext_isdigit(curr_char)) {
					state = EDLX_STATE_NUMBER;
				} else {
					addcurrchar = 0;
					done = 1;
				}
				break;

			case EDLX_STATE_DELIM:
				addcurrchar = 0;
				done = 1;
				break;

			case EDLX_STATE_THIS_LINE:
				addcurrchar = 0;
				done = 1;
				break;

			case EDLX_STATE_COMMAND:
				if(ext_isalnum(curr_char)) {
					state = EDLX_STATE_TEXT;
				} else {
					addcurrchar = 0;
					done = 1;
				}
				break;

			case EDLX_STATE_TEXT:
				if(ext_isalnum(curr_char)) {
					state = EDLX_STATE_TEXT;
				} else {
					addcurrchar = 0;
					done = 1;
				}
				break;

			case EDLX_STATE_ASK:
				if(curr_char == 'R') {
					state = EDLX_STATE_ASK_REPLACE;
					skip = 1;
					done = 1;
				} else if(curr_char == 'S') {
					state = EDLX_STATE_ASK_SEARCH;
					skip = 1;
					done = 1;
				} else {
					state = EDLX_STATE_ASK;
					addcurrchar = 0;
					done = 1;
				}
				break;

			case EDLX_STATE_STRING:
				if(curr_char == '\0') {
					state = EDLX_STATE_INVALID;
				} else if(curr_char == '"') {
					state = EDLX_STATE_STRING_END;
					addcurrchar = 0;
				} else if(curr_char == '\\') {
					state = EDLX_STATE_STRING_ESCAPE;
					addcurrchar = 0;
				}
				break;

			case EDLX_STATE_STRING_ESCAPE:
				switch(curr_char) {
					case 'a': curr_char = '\a'; break;
					case 'b': curr_char = '\b'; break;
					case 'f': curr_char = '\f'; break;
					case 'n': curr_char = '\n'; break;
					case 'r': curr_char = '\r'; break;
					case 't': curr_char = '\t'; break;
					case 'v': curr_char = '\v'; break;
					case '\\': curr_char = '\\'; break;
					case '\'': curr_char = '\''; break;
					case '\"': curr_char = '\"'; break;
					case '\?': curr_char = '\?'; break;
					default: curr_char = '\\'; break;
				}
				state = EDLX_STATE_STRING;
				break;

			case EDLX_STATE_STRING_END:
				addcurrchar = 0;
 				done = 1;
				break;

			case EDLX_STATE_INVALID:
				addcurrchar = 0;
				done = 1;
				break;
		}

		if(addcurrchar) {
			if((status = extend_lexeme(ctx, curr_char)) != RET_OK) {
				ctx->curr_token = EDLX_TOKEN_INVALID;
				return status;
			}
		}
	}

	if(skip == 0 && ctx->curr_lexeme_end > 0)
		ctx->curr_lexeme_end--;

	if(ctx->curr_lexeme == NULL) {
		ctx->curr_lexeme = malloc(1);
		if(ctx->curr_lexeme == NULL)
			return RET_ERR_MALLOC;
		ctx->curr_lexeme[0] = '\0';
	}

	switch(state) {
		case EDLX_STATE_INVALID:			token = EDLX_TOKEN_INVALID;				break;
		case EDLX_STATE_NUMBER:				token = EDLX_TOKEN_NUMBER;				break;
		case EDLX_STATE_TEXT:				token = EDLX_TOKEN_TEXT;				break;
		case EDLX_STATE_ASK:				token = EDLX_TOKEN_KW_ASK;				break;
		case EDLX_STATE_ASK_REPLACE:		token = EDLX_TOKEN_KW_ASK_REPLACE;		break;
		case EDLX_STATE_ASK_SEARCH:			token = EDLX_TOKEN_KW_ASK_SEARCH;		break;
		case EDLX_STATE_STRING_END:			token = EDLX_TOKEN_STRING;				break;
		case EDLX_STATE_THIS_LINE:			token = EDLX_TOKEN_THIS_LINE;			break;
		case EDLX_STATE_EOL:				token = EDLX_TOKEN_EOL;					break;
		case EDLX_STATE_DELIM:				token = get_delim(ctx->curr_lexeme);	break;
		case EDLX_STATE_COMMAND:			token = get_command(ctx->curr_lexeme);	break;
		default:							token = EDLX_TOKEN_ERROR;
	}

	ctx->curr_token = token;
	return RET_OK;
}

int edlx_rewind(edlx_ctx_t *ctx) {
	edlx_ctx_t *last_state;

	if(ctx == NULL) return RET_ERR_NULLPO;
	if(ctx->last_state == NULL) {
		fprintf(stderr, "edlin: Couldn't rewind lexer state. No previous state saved.\n");
		return RET_ERR_NULLPO;
	}

	last_state = ctx->last_state;
	free(ctx->curr_lexeme);

	if((ctx->curr_lexeme = malloc(last_state->curr_lexeme_alloced)) == NULL) {
		fprintf(stderr, "edlin: Couldn't rewind lexer state. Couldn't allocate lexeme.\n");
		return RET_ERR_MALLOC;
	}
	memcpy(ctx->curr_lexeme, last_state->curr_lexeme, last_state->curr_lexeme_size);
	ctx->curr_lexeme_alloced = last_state->curr_lexeme_alloced;
	ctx->curr_lexeme_size = last_state->curr_lexeme_size;
	ctx->curr_lexeme_start = last_state->curr_lexeme_start;
	ctx->curr_lexeme_end = last_state->curr_lexeme_end;
	ctx->curr_token = last_state->curr_token;
	ctx->last_state = NULL;
	
	edlx_ctx_free(last_state);

	return RET_OK;
}

void edlx_print_error(edlx_ctx_t *ctx, const char *errmsg, const int printline, const char *prompt) {
	size_t i;
	size_t add_pad = 0, pointer_pos;

	if(prompt != NULL) {
		add_pad += strlen(prompt);
	} else {
		printf(">");
	}

	if(printline) {
		if(prompt != NULL)
			printf("%s", prompt);
		printf("%s\n", ctx->cmdline);
	}

	pointer_pos = ctx->curr_lexeme_end + add_pad;
	if(pointer_pos > 0) pointer_pos--;

	for(i = 0; i < pointer_pos; i++)
		printf(" ");
	printf("^");

	if(errmsg)
		printf("--- %s\n", errmsg);
	else
		printf("\n");
}

edlx_token_t edlx_get_token(edlx_ctx_t *ctx, int *status) {
	if(status == NULL) return EDLX_TOKEN_ERROR;
	if(ctx == NULL) {
		*status = RET_ERR_NULLPO;
		return EDLX_TOKEN_ERROR;
	}
	*status = RET_OK;
	return ctx->curr_token;
}

int edlx_get_required_token(edlx_ctx_t *ctx, edlx_token_t expect) {
	int status;
	edlx_token_t token;

	if(ctx == NULL) return RET_ERR_NULLPO;

	if((status = edlx_step(ctx)) != RET_OK) return RET_ERR_INTERNAL;
	if((token = edlx_get_token(ctx, &status)) != expect)
		return RET_ERR_SYNTAX;

	return RET_OK;
}

char edlx_get_lookahead(edlx_ctx_t *ctx, int *status) {
	char curr_char;
	size_t seek_pos;

	if(status == NULL) return '\0';
	*status = RET_ERR_NULLPO;
	if(ctx == NULL) return '\0';

	*status = RET_OK;
	seek_pos = ctx->curr_lexeme_end;
	do {
		curr_char = ctx->cmdline[seek_pos++];
	} while(ext_isspace(curr_char) && (curr_char != '\0'));

	return curr_char;
}

char *edlx_get_lexeme(edlx_ctx_t *ctx) {
	if(ctx == NULL) return NULL;
	return ctx->curr_lexeme;
}
