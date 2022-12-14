/*******************************************
 * SPDX-License-Identifier: GPL-2.0-only   *
 * Copyright (C) 2015-2021  Martin Wolters *
 *******************************************/

#include <stdio.h>
#include <string.h>

char *optarg;
int optind = 1, opterr = 1, optopt = '\0';

int getopt(int argc, char *const argv[], const char *optstring) {
	static int nextchar = 1;
	char *found;

	if(!optind) optind = 1;

	do {
		if(!argv[optind] ||
			*argv[optind] != '-')
			return -1;

		if(argv[optind][0] == '-') {
			if(argv[optind][1] == '\0')
				return -1;
			if(argv[optind][1] == '-' &&
				argv[optind][2] == '\0') {
				optind++;
				return -1;
			}
		}

		if(argv[optind][nextchar] != '\0') {
			optopt = argv[optind][nextchar];
			if((found = strchr(optstring, optopt)) != NULL) {
				if(found[0] == '\0') {
					optind++;
					nextchar = 1;
					continue;
				}
				if(found[1] == ':') {
					if(argv[optind][nextchar + 1] == '\0')
						optarg = argv[++optind];
					else
						optarg = argv[optind] + nextchar + 1;

					if(!optarg) {
						if(opterr)
							fprintf(stderr, "%s: option requires an argument -- '%c'\n", argv[0], optopt);
						if(optstring[0] == ':')
							return ':';
						return '?';
					}
				} else {
					nextchar++;
					return optopt;
				}
				optind++;
				nextchar = 1;
				return optopt;
			} else {
				optind++;
				nextchar = 1;
				if(opterr)
					fprintf(stderr, "%s: unrecognised option -- '%c'\n", argv[0], optopt);
				return '?';
			}
		}
		optind++;
		nextchar = 1;
	} while(optind <= argc);

	return -1;
}
