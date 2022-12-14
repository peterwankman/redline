/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifndef ERMAC_H_
#define ERMAC_H_

#define RET_OK					1
#define RET_YES					2
#define RET_NO					3
#define RET_MORE				4

#define RET_ERR_INTERNAL		-1
#define RET_ERR_NULLPO			-2
#define RET_ERR_MALLOC			-3
#define RET_ERR_SYNTAX			-4
#define RET_ERR_NOTFOUND		-5
#define RET_ERR_INVALID			-6
#define RET_ERR_RANGE			-7
#define RET_ERR_OPEN			-8
#define RET_ERR_READ			-9
#define RET_ERR_WRITE			-10
#define RET_ERR_DOUBLE			-100

char *str_error(const int err_no);
int print_error(const int err_no);

#endif