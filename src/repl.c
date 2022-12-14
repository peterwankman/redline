/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ermac.h"
#include "lexer.h"
#include "parser.h"
#include "repl.h"
#include "util.h"

#define PREALLOC_LINES		16

typedef struct repl_state_t {
	int cursor;
	int quit;
	char *search_str;
} repl_state_t;

static void indent(const int n) {
	size_t l = num_len(n);
	size_t i;

	for(i = 0; i < 8 - l; i++)
		printf(" ");
}

static void manual(void) {
	printf("Edit line                   line#\n");
	printf("Append                      [#lines]A\n");
	printf("Copy                        [startline],[endline],toline[,times]C\n");
	printf("Delete                      [startline][,endline]D\n");
	printf("Quit and save changes       E\n");
	printf("Insert                      [line]I\n");
	printf("List                        [startline][,endline]L\n");
	printf("Move                        [startline],[endline],tolineM\n");
	printf("Page                        [startline][,endline]P\n");
	printf("Quit and discard changes    Q\n");
	printf("Search and replace          [startline][,endline][?]Roldtext,newtext\n");
	printf("Search                      [startline][,endline][?]Stext\n");
	printf("Transfer                    [toline]Tfilename\n");
	printf("Write                       [#lines]W\n");
}

static int ask(const char *prompt, FILE *input) {
	int status;
	char reply;

	for(;;) {
		printf("%s (Y/N)? ", prompt);
		fflush(stdout);

		reply = get_key(&status);
		if(status != RET_OK) {
			printf("?\n");
			return RET_ERR_INVALID;
		}
		printf("%c\n", reply);

		if(toupper(reply) == 'Y') return RET_YES;
		if(toupper(reply) == 'N') return RET_NO;
	}
	return 0;
}

/*/*/

static char *text_prompt(const uint32_t line_number) {
	char *read_line;

	indent(line_number);
	printf("%d:*", line_number);

	if((read_line = get_line(stdin)) == NULL) {
		print_error(RET_ERR_MALLOC, "edlin");
		return NULL;
	}

	return read_line;
}

static int is_empty(const char *in) {
	if(strlen(in) != 1) return RET_NO;
	if(in[0] == '.') return RET_YES;
	return RET_NO;
}

/**/

static int append(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t n_lines = 0xffffffff, curr_line;
	char *entered_line;

	if((instr->start_line != EDPS_NO_LINE) ||
	   (instr->end_line != EDPS_NO_LINE) ||
	   (instr->only_line == EDPS_THIS_LINE))
		return print_error(RET_ERR_INVALID, "edlin");

	if(instr->only_line != EDPS_NO_LINE) {
		/* The parser returns a zero indexed line
		 * number, but we don't really mean a
		 * line number here, we mean the number
		 * of lines to append.
		 */
		n_lines = instr->only_line + 1;
	}

	curr_line = document->n_lines + 1;
	do {
		entered_line = text_prompt(curr_line);

		if(is_empty(entered_line) == RET_YES)
			break;

		dynarr_append(document->lines_arr, &entered_line);
		document->n_lines++;

		curr_line++;
		n_lines--;
	} while(n_lines);

	return RET_OK;
}

static int copy(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line;
	uint32_t end = instr->end_line;
	uint32_t target = instr->target_line;
	size_t i, rep, copy_size, count = 0, skip = 1;
	char **read_element, *write_element;

	if(instr->start_line == EDPS_THIS_LINE) start = state->cursor;
	if(instr->end_line == EDPS_THIS_LINE) end = state->cursor;
	if(instr->target_line == EDPS_THIS_LINE) target = state->cursor;

	if(start > end)
		return print_error(RET_ERR_RANGE, "edlin");

	if((target > start) && (target <= end))
		return print_error(RET_ERR_RANGE, "edlin");

	copy_size = end - start + 1;
	if(target <= start) skip = 2;

	for(rep = 0; rep < instr->repeat; rep++) {
		for(i = 0; i < copy_size; i++) {
			read_element = dynarr_get_element(document->lines_arr, start + skip * i);
			write_element = str_alloc_copy(*read_element);
			dynarr_insert(document->lines_arr, &write_element, target + count++);
			document->n_lines++;
		}
	}

	state->cursor = target;
	return RET_OK;
}

static int delete(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line, end = instr->end_line;

	if(instr->start_line == EDPS_THIS_LINE) start = state->cursor;
	if(instr->start_line == EDPS_NO_LINE) start = 0;
	if(instr->end_line == EDPS_THIS_LINE) end = state->cursor;
	if(instr->end_line == EDPS_NO_LINE) end = document->n_lines;

	if(((instr->start_line == EDPS_NO_LINE) && (instr->end_line == EDPS_NO_LINE)) ||
	    (instr->only_line == EDPS_THIS_LINE)) {
		start = state->cursor;
		end = state->cursor;
	}

	if(instr->only_line != EDPS_NO_LINE) {
		start = instr->only_line;
		end = instr->only_line;
	}

	dynarr_delete(document->lines_arr, start, end);
	document->n_lines -= (end - start + 1);

	return RET_OK;
}

static int end(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	int status;

	if((status = save_doc(document, NULL, 0, document->n_lines)) == RET_OK) {
		state->quit = 1;
	} else {
		print_error(status, "edlin");
	}

	return status;
}

static int edit(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	size_t n_line = instr->only_line;
	char **line_str, *new_line;

	if(n_line == EDPS_NO_LINE)
		return print_error(RET_ERR_SYNTAX, "edlin");

	if(n_line > document->n_lines - 1)
		return print_error(RET_ERR_INVALID, "edlin");

	state->cursor = n_line;

	if((line_str = dynarr_get_element(document->lines_arr, n_line)) == NULL)
		return print_error(RET_ERR_NULLPO, "edlin");;

	indent(n_line + 1);
	printf("%ld:*%s\n", n_line + 1, *line_str);
	if((is_empty(new_line = text_prompt(n_line + 1))) == RET_NO) {
		dynarr_insert(document->lines_arr, &new_line, n_line + 1);
		dynarr_delete(document->lines_arr, n_line, n_line);
	} else {
		free(new_line);
	}

	return RET_OK;
}

static int insert(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t l = instr->only_line;
	char *read_line;
	int status, goon = 1;

	if((instr->start_line != EDPS_NO_LINE) || (instr->end_line != EDPS_NO_LINE)) {
		return print_error(RET_ERR_RANGE, "edlin");
	}

	if((instr->only_line == EDPS_NO_LINE) || (instr->only_line == EDPS_THIS_LINE))
		l = state->cursor;

	do {
		read_line = text_prompt(l + 1);

		if(is_empty(read_line) == RET_YES) {
			free(read_line);
			goon = 0;
		} else {
			if((status = dynarr_insert(document->lines_arr, &read_line, l)) != RET_OK) {
				print_error(status, "edlin");
				free(read_line);
				return status;
			} else {
				document->n_lines++;
			}
		}
		l++;
	} while(goon == 1);

	return RET_OK;
}

static int list(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line, end = instr->end_line;
	char **line;
	size_t i;
	uint32_t lines_shown = 0;

	/* L command behaviour:
	 * No arguments:
	 *		List the first 24 lines, starting 11 lines before
	 *		the cursor, ending 12 lines after the cursor.
	 *		If the cursor is less than 11 lines into the file,
	 *		push the end line back to maintain 24 lines.
	 * One argument:
	 *		1. (#L)  No comma? -> List 24 lines, starting with arg line.
	 *		2. (#,L) Start, but no end line? Same as above.
	 *		3. (,#l) End, but no start line?
	 *			if the specified line is within 11 lines before the
	 *			cursor, start there and list until the specified end.
	 *			If not, start the list at the specified line, until
	 *			12 lines after the cursor.
	 *		Why? Don't as me.
	 */

	/* No args case */
	if((instr->only_line == EDPS_NO_LINE) &&
	   (instr->start_line == EDPS_NO_LINE) &&
	   (instr->end_line == EDPS_NO_LINE)) {
		if(state->cursor < 12) {
			start = 0;
		} else {
			start = state->cursor - 11;
		}
		end = start + 23;
	/* One arg, first case (#L) */
	} else if(instr->only_line != EDPS_NO_LINE) {
		if(instr->only_line == EDPS_THIS_LINE) {
			start = state->cursor;
		} else {
			start = instr->only_line;
		}
		end = start + 23;
	/* One arg, second case (#,L) */
	} else if((instr->start_line != EDPS_NO_LINE) &&
		(instr->end_line == EDPS_NO_LINE)) {
		if(instr->start_line == EDPS_THIS_LINE) {
			start = state->cursor;
		} else {
			start = instr->start_line;
		}
		end = start + 23;
	/* One arg, third case (,#L) */
	} else if((instr->start_line == EDPS_NO_LINE) &&
		(instr->end_line != EDPS_NO_LINE)) {
		if(instr->end_line == EDPS_THIS_LINE)
			instr->end_line = state->cursor;

		if(instr->end_line >= state->cursor - 11) {
			if(state->cursor < 12)
				start = 0;
			else
				start = state->cursor - 11;
			end = instr->end_line;
		} else {
			start = instr->end_line;
			end = state->cursor + 12;
		}
	}

	if(end > document->n_lines - 1)
		end = document->n_lines - 1;

	for(i = start; i < end + 1; i++) {
		line = dynarr_get_element(document->lines_arr, i);
		indent(i + 1);
		printf("%ld:", i + 1);
		if(i == state->cursor)
			printf("*");
		else
			printf(" ");
		printf("%s\n", *line);

		lines_shown++;

		if((lines_shown == 24) && (i != end)){
			if(ask("Continue", stdin) == RET_NO) return RET_OK;
			lines_shown = 0;
		}
	}

	return RET_OK;
}

static int move(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line;
	uint32_t end = instr->end_line;
	uint32_t target = instr->target_line;
	size_t move_range;
	int status;

	if(instr->start_line == EDPS_THIS_LINE) start = state->cursor;
	if(instr->end_line == EDPS_THIS_LINE) end = state->cursor;

	if((target >= start) && (target <= end))
		return print_error(RET_ERR_RANGE, "edlin");

	move_range = end - start + 1;

	if(target > end)
		target -= move_range;

	state->cursor = target;
	if((status = dynarr_move(document->lines_arr, start, end, target)) != RET_OK)
		return print_error(status, "edlin");

	return RET_OK;
}

static int page(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line, end = instr->end_line;
	char **line;
	size_t i;
	uint32_t lines_shown = 0;

	/* P command behaviour:
	*
	* P behaves like L, with the default line being
	* the cursor, instead of 11 lines before it.
	*
	* No arguments:
	*		cursor == 1:
	*			start = cursor
	*		else
	*			start = cursor + 1;
	*		end = cursor + 22;
	* One argument:
	*		(#P): start = #; end = start + 22
	*		(#,P): same as 1bove
	*		(,#P): start = cursor + 1; end = #
	*/

	/* No args case (P) */
	if((instr->only_line == EDPS_NO_LINE) &&
		(instr->start_line == EDPS_NO_LINE) &&
		(instr->end_line == EDPS_NO_LINE)) {
		if(state->cursor == 0) {
			start = 0;
		} else {
			start = state->cursor + 1;
		}
		end = start + 22;
	/* One arg, first case (#P) */
	} else if(instr->only_line != EDPS_NO_LINE) {
		if(instr->only_line == EDPS_THIS_LINE) {
			start = state->cursor;
		} else {
			start = instr->only_line;
		}
		end = start + 22;
	/* One arg, second case (#,P) */
	} else if((instr->start_line != EDPS_NO_LINE) &&
		(instr->end_line == EDPS_NO_LINE)) {
		if(instr->start_line == EDPS_THIS_LINE) {
			start = state->cursor;
		} else {
			start = instr->start_line;
		}
		end = start + 22;
	/* One arg, third case (,#P) */
	} else if((instr->start_line == EDPS_NO_LINE) &&
		(instr->end_line != EDPS_NO_LINE)) {
		start = state->cursor;
		if(state->cursor != 0)
			start++;
		end = instr->end_line;
	}

	if(end > document->n_lines - 1)
		end = document->n_lines - 1;

	for(i = start; i < end + 1; i++) {
		line = dynarr_get_element(document->lines_arr, i);
		indent(i + 1);
		printf("%ld:", i + 1);
		if(i == end)
			printf("*");
		else
			printf(" ");
		printf("%s\n", *line);

		lines_shown++;
		state->cursor = i;

		if((lines_shown == 24) && (i != end)) {
			if(ask("Continue", stdin) == RET_NO) return RET_OK;
			lines_shown = 0;
		}
	}

	return RET_OK;
}

static int quit(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	if(ask("Abort edit?", stdin) == RET_YES) state->quit = 1;
	return RET_OK;
}

static char *construct_replace(const char *str, const char *search, const char *replace, size_t * match_pos) {
	char *match;
	size_t out_str_size;
	char *out;

	if(match_pos == NULL) return NULL;

	if((match = strstr(str + *match_pos, search)) == NULL)
		return NULL;

	*match_pos = match - str;

	out_str_size = strlen(str) + strlen(replace) - strlen(search) + 1;
	if((out = malloc(out_str_size)) == NULL) return NULL;

	memcpy(out, str, *match_pos);
	memcpy(out + *match_pos, replace, strlen(replace));
	memcpy(out + *match_pos + strlen(replace),
		str + *match_pos + strlen(search), strlen(str + *match_pos + strlen(search)));
	out[out_str_size - 1] = '\0';

	return out;
}

static int replace(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line, end = instr->end_line;
	char **line;
	size_t i, match_pos = 0;
	int match_status;
	char *edited_str;
	int found = 0;

	start = instr->start_line;
	if(instr->start_line == EDPS_THIS_LINE) start = state->cursor;
	if(instr->start_line == EDPS_NO_LINE) start = state->cursor + 1;
	if(instr->end_line == EDPS_THIS_LINE) end = state->cursor + 1;
	if(instr->end_line == EDPS_NO_LINE) end = start;

	if((instr->start_line == EDPS_NO_LINE) && (instr->end_line == EDPS_THIS_LINE))
		return RET_ERR_SYNTAX;

	if((instr->start_line == EDPS_NO_LINE) && (instr->end_line == EDPS_NO_LINE)) {
		start = state->cursor + 1;
		end = document->n_lines - 1;
	}

	if((instr->replace_str == NULL) || (strlen(instr->replace_str) == 0))
		return print_error(RET_ERR_SYNTAX, "edlin");

	if(state->search_str == NULL) {
		/* Nothing searched before.        */
		/* Empty search string is invalid. */
		if((instr->search_str == NULL) || (instr->search_str[0] == '\0')) {
			fprintf(stderr, "edlin: Not found.\n");
			return RET_ERR_SYNTAX;
		}
		if((state->search_str = str_alloc_copy(instr->search_str)) == NULL)
			return RET_ERR_MALLOC;
	} else {
		/* Search string already set. */
		/* Update the string          */
		if((instr->search_str != NULL) && (instr->search_str[0] != '\0')) {
			free(state->search_str);
			if((state->search_str = str_alloc_copy(instr->search_str)) == NULL)
				return RET_ERR_MALLOC;
		}
	}

	if(end > document->n_lines)
		end = document->n_lines;

	for(i = start; i < end; i++) {
		line = dynarr_get_element(document->lines_arr, i);

		match_pos = 0;
		do {
			if((edited_str = construct_replace(*line, state->search_str, instr->replace_str, &match_pos)) != NULL) {
				found = 1;
				indent(i + 1);
				printf("%ld: %s\n", i + 1, edited_str);

				if(instr->ask == RET_YES) {
					if(ask("O.K.", stdin) == RET_YES) {
						free(*line);
						*line = edited_str;
						match_pos += strlen(instr->replace_str);
					} else {
						match_pos += strlen(state->search_str);
					}
				} else {
					free(*line);
					*line = edited_str;
					match_pos += strlen(instr->replace_str);
				}
			}
		} while(edited_str != NULL);
		state->cursor = i;

/*
		if(strstr(*line, state->search_str)) {
			found = 1;
			match_pos = 0;
			do {
				if((edited_str = construct_replace(*line, state->search_str, instr->replace_str, &match_pos)) != NULL) {
					indent(i + 1);
					printf("%ld: %s\n", i + 1, edited_str);

					if(instr->ask == RET_YES) {
						if(ask("O.K.", stdin) == RET_YES) {
							free(*line);
							*line = edited_str;
							match_pos += strlen(instr->replace_str);
						} else {
							match_pos += strlen(state->search_str);
						}
					} else {
						free(*line);
						*line = edited_str;
						match_pos += strlen(instr->replace_str);
					}
				}
			} while(edited_str != NULL);
			state->cursor = i;
		}
*/
	}

	if(found == 0)
		fprintf(stderr, "edlin: Not found.\n");

	return RET_ERR_NOTFOUND;
}

static int search(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t start = instr->start_line, end = instr->end_line;
	char **line;
	size_t i;

	start = instr->start_line;
	if(instr->start_line == EDPS_THIS_LINE) start = state->cursor;
	if(instr->start_line == EDPS_NO_LINE) start = state->cursor + 1;
	if(instr->end_line == EDPS_THIS_LINE) end = state->cursor + 1;
	if(instr->end_line == EDPS_NO_LINE) end = start;

	if((instr->start_line == EDPS_NO_LINE) && (instr->end_line == EDPS_THIS_LINE))
		return RET_ERR_SYNTAX;

	if((instr->start_line == EDPS_NO_LINE) && (instr->end_line == EDPS_NO_LINE)) {
		start = state->cursor + 1;
		end = document->n_lines - 1;
	}

	if(state->search_str == NULL) {
		/* Nothing searched before.        */
		/* Empty search string is invalid. */
		if((instr->search_str == NULL) || (instr->search_str[0] == '\0')) {
			fprintf(stderr, "edlin: Not found.\n");
			return RET_ERR_SYNTAX;
		}
		if((state->search_str = str_alloc_copy(instr->search_str)) == NULL)
			return RET_ERR_MALLOC;
	} else {
		/* Search string already set. */
		/* Update the string          */
		if((instr->search_str != NULL) && (instr->search_str[0] != '\0')) {
			free(state->search_str);
			if((state->search_str = str_alloc_copy(instr->search_str)) == NULL)
				return RET_ERR_MALLOC;
		}
	}

	if(end > document->n_lines)
		end = document->n_lines;

	for(i = start; i < end; i++) {
		line = dynarr_get_element(document->lines_arr, i);
		if(strstr(*line, state->search_str)) {
			indent(i + 1);
			printf("%ld: %s\n", i + 1, *line);

			state->cursor = i;

			if(instr->ask == RET_YES) {
				if(ask("O.K.", stdin) == RET_YES) {
					return RET_OK;
				}
			} else {
				return RET_OK;
			}
		}
	}

	fprintf(stderr, "edlin: Not found.\n");

	return RET_ERR_NOTFOUND;
}

static int transfer(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t insert_line;
	ed_doc_t *new_doc;
	size_t n_input_lines, input_line = 0;
	char **input_data, *copy_data;
	FILE *fp;
	int status;
	
	if((instr->start_line != EDPS_NO_LINE) || (instr->end_line != EDPS_NO_LINE)) {
		return print_error(RET_ERR_RANGE, "edlin");
	}
	if(instr->only_line == EDPS_NO_LINE) {
		insert_line = state->cursor;
	} else {
		insert_line = instr->only_line;
	}

	if((fp = fopen(instr->filename, "r")) == NULL)
		return print_error(RET_ERR_OPEN, "edlin");

	if((new_doc = load_doc(fp, NULL)) == NULL) {
		fclose(fp);
		return print_error(RET_ERR_READ, "edlin");
	}

	fclose(fp);

	n_input_lines = new_doc->n_lines;
	for(input_line = 0; input_line < n_input_lines; input_line++) {
		status = RET_ERR_MALLOC;

		if((input_data = dynarr_get_element(new_doc->lines_arr, input_line)) == NULL) goto fail;
		if((copy_data = str_alloc_copy(*input_data)) == NULL) goto fail;
		if((status = dynarr_insert(document->lines_arr, &copy_data, input_line + insert_line)) != RET_OK)
			goto fail;
		document->n_lines++;
	}

	status = RET_OK;
fail:
	free_doc(new_doc);
	return status;
}

static int write(repl_state_t *state, ed_doc_t *document, edps_instr_t *instr) {
	uint32_t end_line;
	char *filename;

	if((instr->start_line != EDPS_NO_LINE) || (instr->end_line != EDPS_NO_LINE))
		return print_error(RET_ERR_RANGE, "edlin");

	if(instr->only_line == EDPS_NO_LINE)
		end_line = document->n_lines;
	else
		end_line = instr->only_line;

	if(instr->filename != NULL)
		filename = instr->filename;
	else
		filename = document->filename;

	save_doc(document, filename, 0, end_line);
}

/*/*/

void free_doc(ed_doc_t *doc) {
	if(doc == NULL) return;
	if(doc->filename != NULL) free(doc->filename);
	if(doc->lines_arr != NULL) dynarr_free(doc->lines_arr);
	free(doc);
}

int save_doc(ed_doc_t *doc, const char *filename, const uint32_t start_line, const uint32_t end_line) {
	FILE *fp;
	const char *out_filename = filename;
	uint32_t n_lines, curr_line;
	size_t pos, bytes_written, to_write;
	int fix_line = 0;
	char **line_data;

	char CHECK_CHAR;

	if(doc == NULL)
		return print_error(RET_ERR_INVALID, "edlin");

	if(out_filename == NULL)
		if((out_filename = doc->filename) == NULL)
			return print_error(RET_ERR_INVALID, "edlin");

	if((fp = fopen(out_filename, "w")) == NULL)
		return print_error(RET_ERR_OPEN, "edlin");

	n_lines = dynarr_get_size(doc->lines_arr);
	for(curr_line = 0; curr_line < n_lines; curr_line++) {

		if(curr_line < start_line) continue;
		if(curr_line >= end_line) break;

		line_data = dynarr_get_element(doc->lines_arr, curr_line);

		if(line_data != NULL) {
			to_write = strlen(*line_data);
			pos = 0;

			while(to_write > 0) {
				bytes_written = fwrite(*(line_data + pos), 1, to_write, fp);
				pos += bytes_written;
				to_write -= bytes_written;
			}
			fputc('\n', fp);
		}
	}

	fclose(fp);

	return RET_OK;
}

ed_doc_t *load_doc(FILE *fp, const char *filename) {
	ed_doc_t *out;
	char *read_line;
	uint32_t line_number = 0;

	if((out = malloc(sizeof(ed_doc_t))) == NULL) return NULL;

	if((out->lines_arr = dynarr_new(sizeof(void *), PREALLOC_LINES, free)) == NULL)
		return NULL;
	out->n_lines = 0;

	while(!feof(fp)) {
		if((read_line = get_line(fp)) == NULL) goto fail;

		dynarr_append(out->lines_arr, &read_line);
		out->n_lines++;
	}

	if(filename == NULL) {
		out->filename = NULL;
	} else if((out->filename = str_alloc_copy(filename)) == NULL) {
		goto fail;
	}

	return out;
fail:
	dynarr_free(out->lines_arr);
	free(out);
	return NULL;
}

ed_doc_t *empty_doc(const char *filename) {
	ed_doc_t *out;

	if((out = malloc(sizeof(ed_doc_t))) == NULL) return NULL;
	if((out->lines_arr = dynarr_new(sizeof(void *), PREALLOC_LINES, free)) == NULL) goto fail;
	if(filename == NULL) {
		out->filename = NULL;
	} else {
		if((out->filename = str_alloc_copy(filename)) == NULL) goto freearr;
	}

	out->n_lines = 0;
	return out;
freearr:
	dynarr_free(out->lines_arr);
fail:
	free(out);
	return NULL;
}

static repl_state_t *repl_init(void) {
	repl_state_t *out;

	if((out = malloc(sizeof(repl_state_t))) == NULL) {
		fprintf(stderr, "edlin: Failed to set up the editor.\n");
		return NULL;
	}

	out->quit = 0;
	out->cursor = 0;
	out->search_str = NULL;

	return out;
}

static void repl_free(repl_state_t *state) {
	if(state == NULL) return;
	if(state->search_str != NULL) free(state->search_str);
	free(state);
}

int repl_main(FILE *input, ed_doc_t *ed_doc, const char *prompt) {
	repl_state_t *repl_state;
	char *cmdline;
	edps_ctx_t *parser_ctx;
	edps_instr_t *instruction;
	int parser_status, status = RET_OK;

	if((repl_state = repl_init()) == NULL)
		return RET_ERR_INTERNAL;

	while(repl_state->quit == 0) {
		printf("%s", prompt);
		cmdline = get_line(input);

		if((parser_ctx = edps_new(cmdline, &status)) == NULL) {
			fprintf(stderr, "edlin: Parser initialization failed.\n");
			continue;
		}

		do {
			if((parser_status = edps_parse(parser_ctx)) != RET_OK) {
				switch(parser_status) {
					case RET_ERR_SYNTAX:
						print_error(parser_status, "edlin");
						continue;

					case RET_ERR_INTERNAL:
					case RET_ERR_MALLOC:
						return print_error(parser_status, "edlin");
						return status;
				}
			}

			if((instruction = edps_get_instr(parser_ctx)) == NULL) {
				fprintf(stderr, "edlin: Couldn't fetch the instruction.\n");
				return status;
			}

			switch(instruction->command) {
				case EDPS_CMD_NONE:
					status = edit(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_APPEND:
					status = append(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_ASK:
					manual();
					break;

				case EDPS_CMD_COPY:
					status = copy(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_DELETE:
					status = delete(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_END:
					status = end(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_INSERT:
					status = insert(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_LIST:
					status = list(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_MOVE:
					status = move(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_PAGE:
					status = page(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_QUIT:
					status = quit(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_REPLACE:
					status = replace(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_SEARCH:
					status = search(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_TRANSFER:
					status = transfer(repl_state, ed_doc, instruction);
					break;

				case EDPS_CMD_WRITE:
					status = write(repl_state, ed_doc, instruction);
					break;

			}
		} while(parser_status == RET_MORE);

		free(cmdline);
		edps_free(parser_ctx);
	}

	repl_free(repl_state);
	return status;
}
