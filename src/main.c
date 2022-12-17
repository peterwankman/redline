/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appinfo.h"
#include "ermac.h"
#include "getopt.h"
#include "lexer.h"
#include "parser.h"
#include "repl.h"
#include "util.h"

static void print_version(void) {
	printf("%s, version %d.%d.%d, Copyright (C) 2022 Martin Wolters.\n",
		APP_NAME, APP_VER_MAJOR, APP_VER_MINOR, APP_VER_REV);
	printf("Licensed under the terms of the GNU General Public License.\n");
	printf("(Version 2.0 of the license only.)\n");
}

static void usage(const char *argv) {
	printf("USAGE: %s [drive:][path]filename [-b] [-c] [-p]\n", argv);
	printf("\t-b\tIgnore End-of-file (CTRL-Z/CTRL-D) characters.\n");
	printf("\t-c\tChange the cursor. Default: \"%s\".\n", DEFAULT_PROMPT);
	printf("\t-h\tPrint this help.\n");
	printf("\t-p\tChange the prompt. Default: \"%s\".\n", DEFAULT_CURSOR);
	printf("\t-v\tPrint version and licensing information.\n");
}

int main(int argc, char **argv) {
	int i, ignore_eof = 0;
	char *prompt = NULL;
	char *filename = NULL;
	char *cursor = NULL;
	ed_doc_t *document;
	FILE *fp;

	while((i = getopt(argc, argv, "bc:hp:v")) != -1) {
		switch(i) {
			case 'b':
				ignore_eof = 1;
				break;

			case 'c':
				cursor = optarg;
				break;

			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;

			case 'p':
				prompt = optarg;
				break;

			case 'v':
				print_version();
				return EXIT_SUCCESS;

			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	if((filename = argv[optind]) == NULL) {
		fprintf(stderr, "File name must be specified.\n");
		return EXIT_FAILURE;
	}

	if((fp = fopen(filename, "rb")) == NULL) {
		if((fp = fopen(filename, "wb")) == NULL) {
			fprintf(stderr, "Couldn't open file '%s'.\n", filename);
			return 0;
		} else {
			printf("New file\n");
			document = empty_doc(filename);
		}
	} else {
		if((document = load_doc(fp, filename)) == NULL) return EXIT_FAILURE;
		fclose(fp);
	}

	repl_main(stdin, document, prompt, cursor);
	free_doc(document);

	return EXIT_SUCCESS;
}
