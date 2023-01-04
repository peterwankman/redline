/*******************************************
 * SPDX-License-Identifier: GPL-2.0-only   *
 * Copyright (C) 2015-2019  Martin Wolters *
 *******************************************/

#include <stdio.h>

#ifndef MEM_BST_H_
#define MEM_BST_H_

typedef struct mt_data_t {
	size_t n;
	int line;
	char *file;
} mt_data_t;

typedef struct mt_node_t {
	struct mt_node_t *left, *right, *parent;
	void *addr;
	mt_data_t *data;
	int balance;
} mt_node_t;

mt_node_t *mt_lookup_node(void *addr);
mt_data_t *mt_lookup(void *addr);
int mt_ins(void *addr, size_t n, int line, char *file);
int mt_del_node(mt_node_t *node);
void mt_printlist(FILE *fp, const int verbose);

#endif
