/*******************************************
 * SPDX-License-Identifier: GPL-2.0-only   *
 * Copyright (C) 2015-2021  Martin Wolters *
 *******************************************/

#ifndef GETOPT_H_
#define GETOPT_H_

int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;

#endif
