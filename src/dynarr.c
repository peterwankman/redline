/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "leak.h"

#include "dynarr.h"
#include "ermac.h"

#define DEFAULT_PREALLOC_SIZE	1
#define DEFAULT_ELEMENT_SIZE	1

typedef struct dynarr_t {
	size_t element_size, prealloc_size;
	size_t n_used, n_alloced;
	void *data;
	dynarr_freefunc_t freefunc;
} dynarr_t;

static void *get_index(dynarr_t *arr, const size_t index) {
	uint8_t *raw_array;

	if(arr == NULL) return NULL;
	if(index > arr->n_used) return NULL;

	raw_array = arr->data;
	return raw_array + arr->element_size * index;
}

static int dynarr_extend(dynarr_t *arr, const size_t n_blocks) {
	size_t old_size, new_size;
	char *new_data;

	old_size = arr->element_size * arr->n_alloced;
	new_size = old_size + n_blocks * arr->element_size;
	if((new_data = malloc(new_size)) == NULL) return RET_ERR_MALLOC;

	memcpy(new_data, arr->data, arr->element_size * arr->n_alloced);
	memset(new_data + old_size, 0, n_blocks * arr->element_size);

	free(arr->data);
	arr->data = new_data;
	arr->n_alloced += n_blocks;

	return RET_OK;
}

/**/

int dynarr_append(dynarr_t *arr, const void *data) {
	void *out_pos;
	int status;

	if(arr == NULL) return RET_ERR_NULLPO;

	if(arr->n_used == arr->n_alloced)
		if((status = dynarr_extend(arr, arr->prealloc_size)) != RET_OK)
			return status;

	if((out_pos = dynarr_get_element(arr, arr->n_used)) != NULL)
		memcpy(out_pos, data, arr->element_size);
	arr->n_used++;

	return RET_OK;
}

 int dynarr_delete(dynarr_t *arr, const size_t start_index, const size_t end_index) {
	size_t actual_start, actual_end;
	size_t del_size, tail_size, i;
	uint8_t *move_from, *move_to;
	void **element;

	if(arr == NULL) return RET_ERR_NULLPO;
	if(start_index > arr->n_used - 1) return RET_ERR_RANGE;
	if(end_index < start_index) return RET_ERR_SYNTAX;

	actual_start = start_index;
	actual_end = end_index;

	if(end_index >= arr->n_used - 1)
		actual_end = arr->n_used - 1;

	del_size = (actual_end - actual_start + 1); /* Include both ends. */
	tail_size = arr->n_used - actual_end - 1;

	for(i = actual_start; i < actual_end + 1; i++) {
		if((element = dynarr_get_element(arr, i)) != NULL)
			arr->freefunc(element);
	}

	move_from = dynarr_get_element(arr, actual_end + 1);
	move_to = dynarr_get_element(arr, actual_start);

	memmove(move_to, move_from, tail_size * arr->element_size);
	arr->n_used -= del_size;

	return RET_OK;
}

int dynarr_insert(dynarr_t *arr, const void *data, const size_t pos) {
	size_t actual_pos = pos;
	uint8_t *bytes;
	void *move_from, *move_to;
	size_t move_size;
	int status;

	if(arr == NULL) return RET_ERR_NULLPO;

	if(arr->n_used == arr->n_alloced)
		if((status = dynarr_extend(arr, arr->prealloc_size)) != RET_OK)
			return status;

	bytes = arr->data;
	if(pos > arr->n_used) actual_pos = arr->n_used;

	if(actual_pos <= arr->n_used) {
		move_from = dynarr_get_element(arr, actual_pos);
		move_to = dynarr_get_element(arr, actual_pos + 1);
		move_size = (arr->n_used - actual_pos) * arr->element_size;
		memmove(move_to, move_from, move_size);
	}

	memcpy(bytes + actual_pos * arr->element_size, data, arr->element_size);
	arr->n_used++;

	return RET_OK;
}

int dynarr_move(dynarr_t *arr, const size_t start_index, const size_t end_index, const size_t target_index) {
	void *buf, *move_from, *move_to;
	size_t buf_size, move_size;
	size_t actual_target = target_index;

	if(arr == NULL) return RET_ERR_NULLPO; 

	if((start_index > arr->n_used - 1) || (end_index > arr->n_used - 1))
		return RET_ERR_NOTFOUND;

	if(target_index + end_index - start_index >= arr->n_used)
		actual_target = arr->n_used + start_index - end_index - 1;
	if(actual_target == start_index) return RET_OK;

	/* Grab whatever we want to move. */
	buf_size = (end_index - start_index + 1) * arr->element_size;
	if((buf = malloc(buf_size)) == NULL) return RET_ERR_MALLOC;
	move_from = dynarr_get_element(arr, start_index);
	memcpy(buf, move_from, buf_size);

	/* Move other stuff out of the way */
	if(actual_target < start_index) {
		/* Move down the list */
		move_size = (start_index - actual_target) * arr->element_size;
		move_from = dynarr_get_element(arr, actual_target);
		move_to = (uint8_t*)move_from + buf_size;
		memmove(move_to, move_from, move_size);
	} else {
		/* Move up the list */
		move_size = (actual_target - start_index) * arr->element_size;
		move_from = dynarr_get_element(arr, end_index + 1);
		move_to = dynarr_get_element(arr, start_index);
		memmove(move_to, move_from, move_size);
	}
	/* Move our block inbetween. */
	move_from = buf;
	move_to = dynarr_get_element(arr, actual_target);
	memcpy(move_to, buf, buf_size);

	free(buf);
	return RET_OK;
}

/**/

dynarr_t *dynarr_new(const size_t chunk_size, const size_t prealloc_size, dynarr_freefunc_t freefunc) {
	dynarr_t *out;

	if((out = malloc(sizeof(dynarr_t))) == NULL) return NULL;

	out->element_size = chunk_size ? chunk_size : DEFAULT_ELEMENT_SIZE;
	out->prealloc_size = prealloc_size ? prealloc_size : DEFAULT_PREALLOC_SIZE;

	if((out->data = malloc(out->element_size * out->prealloc_size)) == NULL) goto fail;
	out->n_alloced = prealloc_size;
	out->n_used = 0;
	out->freefunc = freefunc;

	return out;

fail:
	free(out);
	return NULL;
}

void dynarr_free(dynarr_t *arr) {
	dynarr_freefunc_t freefunc;
	char **element;
	size_t i;

	if(arr == NULL) return;
	freefunc = arr->freefunc;
	for(i = 0; i < arr->n_used; i++) {
		element = dynarr_get_element(arr, i);
		if(freefunc != NULL)
			freefunc(element);
	}

	if(arr->data != NULL)
		free(arr->data);
	free(arr);
}

size_t dynarr_get_size(const dynarr_t *arr) {
	if(arr == NULL) return 0;
	return arr->n_used;
}

void *dynarr_get_element(const dynarr_t *arr, const size_t index) {
	if(arr == NULL) return NULL;
	if(index > arr->n_used) return NULL;

	return (uint8_t *)arr->data + arr->element_size * index;
}

size_t dynarr_get_element_size(const dynarr_t *arr) {
	if(arr == NULL) return 0;
	return arr->element_size;
}