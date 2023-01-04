/*******************************************
 *  SPDX-License-Identifier: GPL-2.0-only  *
 * Copyright (C) 2022-2023  Martin Wolters *
 *******************************************/

#ifndef DYNARR_H_
#define DYNARR_H_

typedef struct dynarr_t dynarr_t;
typedef void (*dynarr_freefunc_t)(void *);

dynarr_t *dynarr_new(const size_t chunk_size, const size_t prealloc_size, dynarr_freefunc_t freefunc);
void dynarr_free(dynarr_t *arr);
int dynarr_append(dynarr_t *arr, const void *data);
int dynarr_delete(dynarr_t *arr, const size_t start_index, const size_t end_index);
int dynarr_insert(dynarr_t *arr, const void *data, const size_t pos);
int dynarr_move(dynarr_t *arr, const size_t start_index, const size_t end_index, const size_t target_index);
size_t dynarr_get_size(const dynarr_t *arr);
void *dynarr_get_element(const dynarr_t *arr, const size_t index);
size_t dynarr_get_element_size(const dynarr_t *arr);

#endif
