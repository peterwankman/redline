/*******************************************
 *  SPDX-License-Identifier: GPL-2.0-only  *
 * Copyright (C) 2022-2023  Martin Wolters *
 *******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "appinfo.h"
#include "ermac.h"
#include "lexer.h"
#include "parser.h"
#include "parser_int.h"
#include "util.h"

/**/

static void print_instr(edps_instr_t *instr) {
	switch(instr->command) {
		case EDPS_CMD_NONE:
			printf("\tCmd: none. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d\n", instr->only_line);
			else
				printf("Line: %d\n", instr->end_line);
			break;
	
		case EDPS_CMD_APPEND:
			printf("\tCmd: Append. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d\n", instr->only_line);
			else
				printf("Line: %d\n", instr->end_line);
			break;

		case EDPS_CMD_ASK:
			printf("\tCmd: Ask.\n");
			break;

		case EDPS_CMD_COPY:
			printf("\tCmd: Copy. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d. ", instr->only_line);
			else
				printf("Lines: %d to %d. ", instr->start_line, instr->end_line);

			printf("Target: %d. ", instr->target_line);
			printf("Repeat: %d.\n", instr->repeat);
			break;

		case EDPS_CMD_DELETE:
			printf("\tCmd: Delete. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d to %d.\n", instr->start_line, instr->end_line);
			break;

		case EDPS_CMD_END:
			printf("\tCmd: End.\n");
			break;

		case EDPS_CMD_INSERT:
			printf("\tCmd: Insert. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d\n", instr->end_line);
			break;

		case EDPS_CMD_LIST:
			printf("\tCmd: List. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d to %d.\n", instr->start_line, instr->end_line);
			break;

		case EDPS_CMD_MOVE:
			printf("\tCmd: Move. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d. ", instr->only_line);
			else
				printf("Line: %d to %d. ", instr->start_line, instr->end_line);
			printf("Target: %d.\n", instr->target_line);
			break;

		case EDPS_CMD_PAGE:
			printf("\tCmd: Page. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d to %d.\n", instr->start_line, instr->end_line);
			break;

		case EDPS_CMD_QUIT:
			printf("\tCommand: Quit.\n");
			break;

		case EDPS_CMD_REPLACE:
			printf("\tCmd: Replace%s. ",
				instr->ask ? " (Interactive)" : "");
			printf("Search: '%s'. ", instr->search_str);
			printf("Replace: '%s'. ", instr->replace_str);
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d to %d.\n", instr->start_line, instr->end_line);
			break;

		case EDPS_CMD_SEARCH:
			printf("\tCommand: Search%s. ",
				instr->ask ? " (Interactive)" : "");
			printf("Search: '%s'. ", instr->search_str);
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d to %d.\n", instr->start_line, instr->end_line);
			break;

		case EDPS_CMD_TRANSFER:
			printf("\tCommand: Transfer. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d. ", instr->only_line);
			if(instr->filename != NULL)
				printf("File: %s.", instr->filename);
			printf("\n");
			break;

		case EDPS_CMD_WRITE:
			printf("\tCommand: Write. ");
			if(instr->only_line != EDPS_NO_LINE)
				printf("Line: %d.\n", instr->only_line);
			else
				printf("Line: %d\n", instr->end_line);
			break;
	}
}

static void edps_instr_free(edps_instr_t *instr) {
	if(instr == NULL) return;

	if(instr->filename != NULL) free(instr->filename);
	if(instr->search_str != NULL) free(instr->search_str);
	if(instr->replace_str != NULL) free(instr->replace_str);

	free(instr);
}

static void instr_reset(edps_instr_t *instr) {
	if(instr->search_str != NULL)
		free(instr->search_str);
	if(instr->replace_str != NULL)
		free(instr->replace_str);

	instr->start_line = EDPS_NO_LINE;
	instr->end_line = EDPS_NO_LINE;
	instr->only_line = EDPS_NO_LINE;
	instr->target_line = EDPS_NO_LINE;
	instr->command = EDPS_CMD_NONE;
	instr->ask = 0;
	instr->repeat = 1;
	instr->search_str = NULL;
	instr->replace_str = NULL;
	instr->filename = NULL;
}

static edps_instr_t *instr_new(void) {
	edps_instr_t *instr;

	if((instr = malloc(sizeof(edps_instr_t))) == NULL) {
		fprintf(stderr, "%s: Failed to set up an instruction context.\n", APP_NAME);
	}

	instr_reset(instr);
	return instr;
}

/**/

static int ps_set_start_range(edps_instr_t *instr, const int line) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_start_range(%d)\n", line);
#endif

	if(line == 0) return RET_ERR_SYNTAX;

	if((instr->start_line != EDPS_NO_LINE) || (instr->only_line != EDPS_NO_LINE)) {
		fprintf(stderr, "Parser: Encountered multiple ranges.\n");
		return print_error(RET_ERR_PARSER);
	}
	instr->start_line = line > 0 ? line - 1 : line;
	return RET_OK;
}

static int ps_set_end_range(edps_instr_t *instr, const int line) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_end_range(%d)\n", line);
#endif

	if(line == 0) return RET_ERR_SYNTAX;

	if((instr->end_line != EDPS_NO_LINE) || (instr->only_line != EDPS_NO_LINE)) {
		fprintf(stderr, "Parser: Encountered multiple ranges.\n");
		return print_error(RET_ERR_PARSER);
	}
	instr->end_line = line > 0 ? line - 1 : line;
	return RET_OK;
}

static int ps_set_only_line(edps_instr_t *instr, const int line) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_only_line(%d)\n", line);
#endif

	if(line == 0) return RET_ERR_SYNTAX;

	if((instr->only_line != EDPS_NO_LINE) ||
	   (instr->start_line != EDPS_NO_LINE) ||
	   (instr->end_line != EDPS_NO_LINE)) {
			fprintf(stderr, "Parser: Encountered multiple ranges.\n");
			return print_error(RET_ERR_PARSER);;
	}
	instr->only_line = line > 0 ? line - 1 : line;
	return RET_OK;
}

static int ps_set_target(edps_instr_t *instr, const int line) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_target(%d)\n", line);
#endif

	if(line == 0) return RET_ERR_SYNTAX;

	if(instr->target_line != EDPS_NO_LINE) {
		fprintf(stderr, "Parser: Encountered multiple targets.\n");
		return print_error(RET_ERR_PARSER);
	}
	instr->target_line = line > 0 ? line - 1 : line;
	return RET_OK;
}

static int ps_set_repeat(edps_instr_t *instr, const int n) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_repeat(%d)\n", n);
#endif

	if(n == 0) return RET_ERR_SYNTAX;

	if(instr->repeat != 1) {
		fprintf(stderr, "Parser: Encountered multiple repetitions.\n");
		return print_error(RET_ERR_PARSER);
	}
	instr->repeat = n;
	return RET_OK;
}

static int ps_set_command(edps_instr_t *instr, const edps_cmd_t command) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_command(%d)\n", command);
#endif

	if(instr->command != EDPS_CMD_NONE) {
		fprintf(stderr, "Parser: Encountered multiple commands.\n");
		return print_error(RET_ERR_PARSER);
	}
	instr->command = command;
	return RET_OK;
}

static int ps_set_ask(edps_instr_t *instr) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_ask()\n");
#endif

	if(instr->ask != 0) {
		fprintf(stderr, "Parser: Encountered multiple commands.\n");
		return print_error(RET_ERR_PARSER);
	}
	instr->ask = RET_YES;
	return RET_OK;
}

static int ps_set_search(edps_instr_t *instr, const char *search_str) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_search(\"%s\")\n", search_str);
#endif

	if(instr->search_str != NULL) {
		fprintf(stderr, "Parser: Encountered multiple search strings.\n");
		return print_error(RET_ERR_PARSER);
	}

	if(search_str != NULL) {
		if((instr->search_str = str_alloc_copy(search_str)) == NULL) {
			fprintf(stderr, "Parser: Couldn't save the search string.\n");
			return print_error(RET_ERR_MALLOC);
		}
	} else {
		instr->search_str = NULL;
	}
	return RET_OK;
}

static int ps_set_replace(edps_instr_t *instr, const char *replace_str) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_replace(\"%s\")\n", replace_str);
#endif

	if(instr->replace_str != NULL) {
		fprintf(stderr, "Parser: Encountered multiple replacement strings.\n");
		return print_error(RET_ERR_PARSER);
	}

	if(replace_str != NULL) {
		if((instr->replace_str = str_alloc_copy(replace_str)) == NULL) {
			fprintf(stderr, "Parser: Couldn't save the replacement string.\n");
			return print_error(RET_ERR_MALLOC);
		}
	} else {
		instr->replace_str = NULL;
	}
	return RET_OK;
}

static int ps_set_filename(edps_instr_t *instr, const char *filename_str) {
#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_set_filename(\"%s\")\n", filename_str);
#endif

	if(instr->filename != NULL) {
		fprintf(stderr, "Parser: Encountered multiple input files.\n");
		return print_error(RET_ERR_PARSER);
	}

	if(filename_str != NULL) {
		if((instr->filename = str_alloc_copy(filename_str)) == NULL) {
			fprintf(stderr, "Parser: Couldn't save the input filename.\n");
			return print_error(RET_ERR_MALLOC);
		}
	} else {
		instr->filename = NULL;
	}
	return RET_OK;
}

/**/

static int ps_after_range(edps_ctx_t *ctx) {
	edlx_token_t token;
	char lookahead;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_after_range()\n");
#endif

	lookahead = edlx_get_lookahead(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(lookahead) {
		case ',':
			if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
				return status;
			return ps_target(ctx);

		case ';':
			return ps_set_command(ctx->instr, EDPS_CMD_EDIT);
	}

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_KW_APPEND:
			status = ps_set_command(ctx->instr, EDPS_CMD_APPEND);
			break;

		case EDLX_TOKEN_KW_DELETE:
			status = ps_set_command(ctx->instr, EDPS_CMD_DELETE);
			break;

		case EDLX_TOKEN_KW_INSERT:
			status = ps_set_command(ctx->instr, EDPS_CMD_INSERT);
			break;

		case EDLX_TOKEN_KW_LIST:
			status = ps_set_command(ctx->instr, EDPS_CMD_LIST);
			break;

		case EDLX_TOKEN_KW_PAGE:
			status = ps_set_command(ctx->instr, EDPS_CMD_PAGE);
			break;

		case EDLX_TOKEN_EOL:
			status = ps_set_command(ctx->instr, EDPS_CMD_EDIT);
			break;

		case EDLX_TOKEN_KW_TRANSFER:
			if((status = ps_set_command(ctx->instr, EDPS_CMD_TRANSFER)) != RET_OK)
				break;
			status = ps_transfer(ctx);
			break;

		case EDLX_TOKEN_KW_WRITE:
			if((status = ps_set_command(ctx->instr, EDPS_CMD_WRITE)) != RET_OK)
				break;
			status = ps_write(ctx);
			break;

		case EDLX_TOKEN_KW_ASK_SEARCH:
		case EDLX_TOKEN_KW_SEARCH:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_search(ctx);
			break;

		case EDLX_TOKEN_KW_ASK_REPLACE:
		case EDLX_TOKEN_KW_REPLACE:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_replace(ctx);
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	return status;
}

static int ps_copy(edps_ctx_t *ctx) {
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_copy()\n");
#endif

	if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_KW_COPY)) != RET_OK)
		return status;

	return ps_set_command(ctx->instr, EDPS_CMD_COPY);
}

static int ps_move(edps_ctx_t *ctx) {
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_move()\n");
#endif

	if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_KW_MOVE)) != RET_OK)
		return status;

	return ps_set_command(ctx->instr, EDPS_CMD_MOVE);
}

static int ps_range_end(edps_ctx_t *ctx) {
	edlx_token_t token;
	char *lexeme;
	int status;
	int cmd_parsed = 0;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_range_end()\n");
#endif

	if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_DELIM_COMMA)) != RET_OK)
		return status;

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	lexeme = edlx_get_lexeme(ctx->edlx_ctx);

	switch(token) {
		case EDLX_TOKEN_THIS_LINE:
			status = ps_set_end_range(ctx->instr, EDPS_THIS_LINE);
			break;

		case EDLX_TOKEN_NUMBER:
			if(is_good_integer(lexeme) == RET_NO) {
				status = RET_ERR_OVERFLOW;
				break;
			}
			status = ps_set_end_range(ctx->instr, atoi(lexeme));
			break;

		case EDLX_TOKEN_KW_COPY:
		case EDLX_TOKEN_KW_DELETE:
		case EDLX_TOKEN_KW_LIST:
		case EDLX_TOKEN_KW_MOVE:
		case EDLX_TOKEN_KW_PAGE:
		case EDLX_TOKEN_KW_REPLACE:
		case EDLX_TOKEN_KW_SEARCH:
		case EDLX_TOKEN_KW_TRANSFER:
			edlx_rewind(ctx->edlx_ctx);
			if((status = ps_after_range(ctx)) == RET_OK)
				cmd_parsed = 1;
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	/* This is simple enough to check in the parser. */
	if((ctx->instr->end_line > -1) && (ctx->instr->end_line < ctx->instr->start_line))
		status = RET_ERR_SYNTAX;

	if(status != RET_OK) return status;

	if(cmd_parsed == 0)
		return ps_after_range(ctx);
	else
		return status;
}

static int ps_range_start(edps_ctx_t *ctx) {
	edlx_token_t token;
	char *lexeme;
	int status, end_of_range = 0;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_range_start()\n");
#endif

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	lexeme = edlx_get_lexeme(ctx->edlx_ctx);
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	/* If the line number isn't followed by a comma, it is the end of the range. */
	if(edlx_get_lookahead(ctx->edlx_ctx, &status) != ',') end_of_range = 1;
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_THIS_LINE:
			if(end_of_range == 0)
				status = ps_set_start_range(ctx->instr, EDPS_THIS_LINE);
			else
				status = ps_set_only_line(ctx->instr, EDPS_THIS_LINE);
			break;

		case EDLX_TOKEN_NUMBER:
			if(is_good_integer(lexeme) == RET_NO) {
				status = RET_ERR_OVERFLOW;
				break;
			}
			if(end_of_range == 0)
				status = ps_set_start_range(ctx->instr, atoi(lexeme));
			else
				status = ps_set_only_line(ctx->instr, atoi(lexeme));
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	if(status != RET_OK) return status;

	if(end_of_range == 0)
		status = ps_range_end(ctx);
	else
		status = ps_after_range(ctx);
	return status;
}

static int ps_repeat(edps_ctx_t *ctx) {
	char *lexeme;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_repeat()\n");
#endif

	if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_NUMBER)) != RET_OK)
		return status;
	lexeme = edlx_get_lexeme(ctx->edlx_ctx);
	if(is_good_integer(lexeme) == RET_NO)
		return RET_ERR_OVERFLOW;

	if((status = ps_set_repeat(ctx->instr, atoi(lexeme))) != RET_OK)
		return status;

	return ps_copy(ctx);
}

static int ps_replace(edps_ctx_t *ctx) {
	edlx_token_t token;
	char *lexeme;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_replace()\n");
#endif

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_KW_ASK_REPLACE:
			if((status = ps_set_ask(ctx->instr)) != RET_OK)
				return status;
		case EDLX_TOKEN_KW_REPLACE:
			status = ps_set_command(ctx->instr, EDPS_CMD_REPLACE);
			break;

		default:
			status = RET_ERR_PARSER;
	}

	if(status != RET_OK) return status;

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		/* If the search string is omitted, make it the empty string */
		case EDLX_TOKEN_DELIM_COMMA:
			if((status = ps_set_search(ctx->instr, "")) != RET_OK)
				return status;
			edlx_rewind(ctx->edlx_ctx);
			break;

		case EDLX_TOKEN_STRING:
			lexeme = edlx_get_lexeme(ctx->edlx_ctx);
			if((status = ps_set_search(ctx->instr, lexeme)) != RET_OK)
				return status;
			break;
	}

	if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_DELIM_COMMA)) != RET_OK)
		return status;

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_STRING:
			lexeme = edlx_get_lexeme(ctx->edlx_ctx);
			status = ps_set_replace(ctx->instr, lexeme);
			break;

		case EDLX_TOKEN_DELIM_COMMA:
		case EDLX_TOKEN_EOL:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_set_replace(ctx->instr, "");
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	return status;
}

static int ps_search(edps_ctx_t *ctx) {
	edlx_token_t token;
	char *lexeme, lookahead;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_search()\n");
#endif

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_KW_ASK_SEARCH:
			if((status = ps_set_ask(ctx->instr)) != RET_OK)
				return status;

		case EDLX_TOKEN_KW_SEARCH:
			if((lookahead = edlx_get_lookahead(ctx->edlx_ctx, &status)) == '\"') {
				if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_STRING)) != RET_OK)
					return RET_ERR_SYNTAX;
				lexeme = edlx_get_lexeme(ctx->edlx_ctx);
			} else {
				lexeme = NULL;
			}

			if((status = ps_set_search(ctx->instr, lexeme)) != RET_OK)
				return status;
			if((status = ps_set_command(ctx->instr, EDPS_CMD_SEARCH)) != RET_OK)
				return status;
			break;

		default:
			status = RET_ERR_PARSER;
	}

	return status;
}

static int ps_standalone_cmd(edps_ctx_t *ctx) {
	edlx_token_t token;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_standalone_cmd()\n");
#endif

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_KW_ASK:
			status = ps_set_command(ctx->instr, EDPS_CMD_ASK);
			break;

		case EDLX_TOKEN_KW_END:
			status = ps_set_command(ctx->instr, EDPS_CMD_END);
			break;

		case EDLX_TOKEN_KW_QUIT:
			status = ps_set_command(ctx->instr, EDPS_CMD_QUIT);
			break;
	}

	return status;
}

static int ps_statement(edps_ctx_t *ctx) {
	edlx_token_t token;
	char lookahead;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_statement()\n");
#endif

	lookahead = edlx_get_lookahead(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;
	if(lookahead == ';') return edlx_step(ctx->edlx_ctx);

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_THIS_LINE:
		case EDLX_TOKEN_NUMBER:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_range_start(ctx);
			break;

		case EDLX_TOKEN_DELIM_COMMA:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_range_end(ctx);
			break;

		case EDLX_TOKEN_KW_ASK_REPLACE:
		case EDLX_TOKEN_KW_REPLACE:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_replace(ctx);
			break;

		case EDLX_TOKEN_KW_ASK_SEARCH:
		case EDLX_TOKEN_KW_SEARCH:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_search(ctx);
			break;

		case EDLX_TOKEN_KW_APPEND:
		case EDLX_TOKEN_KW_COPY:
		case EDLX_TOKEN_KW_DELETE:
		case EDLX_TOKEN_KW_INSERT:
		case EDLX_TOKEN_KW_LIST:
		case EDLX_TOKEN_KW_MOVE:
		case EDLX_TOKEN_KW_PAGE:
		case EDLX_TOKEN_KW_TRANSFER:
		case EDLX_TOKEN_KW_WRITE:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_after_range(ctx);
			break;

		case EDLX_TOKEN_KW_ASK:
		case EDLX_TOKEN_KW_END:
		case EDLX_TOKEN_KW_QUIT:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_standalone_cmd(ctx);
			break;

		case EDLX_TOKEN_EOL:
			status = RET_OK;
			break;

		default:
			return RET_ERR_SYNTAX;
	}

	if(status != RET_OK) return status;

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_DELIM_SEMICOLON:
			ctx->n_subexpr++;
		case EDLX_TOKEN_EOL:
			edlx_rewind(ctx->edlx_ctx);
			status = RET_OK;
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	return status;
}

static int ps_target(edps_ctx_t *ctx) {
	edlx_token_t token;
	char *lexeme;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_target()\n");
#endif

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;

	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;
	lexeme = edlx_get_lexeme(ctx->edlx_ctx);

	switch(token) {
		case EDLX_TOKEN_THIS_LINE:
			status = ps_set_target(ctx->instr, EDPS_THIS_LINE);
			break;

		case EDLX_TOKEN_NUMBER:
			if(is_good_integer(lexeme) == RET_NO) {
				status = RET_ERR_OVERFLOW;
				break;
			}
			status = ps_set_target(ctx->instr, atoi(lexeme));
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	if(status != RET_OK) return status;

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_DELIM_COMMA:
			status = ps_repeat(ctx);
			break;

		case EDLX_TOKEN_KW_COPY:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_copy(ctx);
			break;

		case EDLX_TOKEN_KW_MOVE:
			edlx_rewind(ctx->edlx_ctx);
			status = ps_move(ctx);
			break;

		default:
			return RET_ERR_SYNTAX;
	}

	return status;
}

static int ps_transfer(edps_ctx_t *ctx) {
	char *lexeme;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_transfer()\n");
#endif

	if((status = edlx_get_required_token(ctx->edlx_ctx, EDLX_TOKEN_STRING)) != RET_OK)
		return status;
	lexeme = edlx_get_lexeme(ctx->edlx_ctx);
	return ps_set_filename(ctx->instr, lexeme);
}

static int ps_write(edps_ctx_t *ctx) {
	edlx_token_t token;
	char lookahead, *lexeme;
	int status;

#ifdef DEBUG_VERBOSE
	printf("PARSER: ps_write()\n");
#endif

	lookahead = edlx_get_lookahead(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	if(lookahead == ';') return RET_OK;

	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	switch(token) {
		case EDLX_TOKEN_STRING:
			lexeme = edlx_get_lexeme(ctx->edlx_ctx);
			status = ps_set_filename(ctx->instr, lexeme);
			break;

		case EDLX_TOKEN_EOL:
			status = RET_OK;
			break;

		default:
			status = RET_ERR_SYNTAX;
	}

	return status;
}

int edps_parse(edps_ctx_t *ctx) {
	int done = 0;
	int status;
	int token;

	if(ctx == NULL) return RET_ERR_NULLPO;

	instr_reset(ctx->instr);

	if((status = ps_statement(ctx)) != RET_OK) {
		if(status == RET_ERR_SYNTAX)
			edlx_print_error(ctx->edlx_ctx, "Syntax error.", ctx->n_subexpr, ctx->prompt);
		return status;
	}
#ifdef DEBUG_VERBOSE
	print_instr(ctx->instr);
#endif
	if((status = edlx_step(ctx->edlx_ctx)) != RET_OK)
		return status;
	token = edlx_get_token(ctx->edlx_ctx, &status);
	if(status != RET_OK) return status;

	if(token == EDLX_TOKEN_EOL)
		return RET_OK;

	return RET_MORE;
}

/**/

void edps_free(edps_ctx_t *ctx) {
	if(ctx == NULL) return;

	if(ctx->edlx_ctx != NULL) edlx_ctx_free(ctx->edlx_ctx);
	if(ctx->instr != NULL) edps_instr_free(ctx->instr);
	free(ctx);
}

edps_ctx_t *edps_new(const char *cmdline, const char *prompt, int *status) {
	edps_ctx_t *ctx;

	if(status == NULL) return NULL;

	*status = RET_ERR_MALLOC;
	if((ctx = malloc(sizeof(edps_ctx_t))) == NULL) {
		fprintf(stderr, "%s: Failed to set up a parser context.\n", APP_NAME);
		return NULL;
	}

	if((ctx->edlx_ctx = edlx_ctx_new(cmdline, status)) == NULL) {
		fprintf(stderr, "%s: Failed to set up a lexer context.\n", APP_NAME);
		free(ctx);
		return NULL;
	}

	if((ctx->instr = instr_new()) == NULL) {
		edlx_ctx_free(ctx->edlx_ctx);
		free(ctx);
		return NULL;
	}

	ctx->prompt = prompt;

	ctx->n_subexpr = 0;
	*status = RET_OK;
	return ctx;
}

edps_instr_t *edps_get_instr(edps_ctx_t *ctx) {
	return ctx->instr;
}
