/*******************************************
 * SPDX-License-Identifier: GPL-2.0-only   *
 * Copyright (C) 2015-2023  Martin Wolters *
 *******************************************/

#include <stdio.h>
#include <stdlib.h>

#ifndef MEM_H_
#define MEM_H_

#ifdef _DEBUG
#define malloc(ptr) mem_alloc(ptr, __FILE__, __LINE__)
#define calloc(num, size) mem_calloc(num, size, __FILE__, __LINE__)
#define realloc(ptr, n) mem_realloc(ptr, n, __FILE__, __LINE__)
#define free mem_free

#define DISABLE_CRT_LEAKCHECK

#endif

void *mem_alloc(const size_t n, const char *file, const int line);
void *mem_calloc(const size_t num, const size_t size, const char *file, const int line);
void *mem_realloc(void *ptr, const size_t n, const char *file, const int line);
void mem_free(void *ptr);
size_t get_mem_allocated(void);
void mem_summary(FILE *fp, const int verbose);

#endif
