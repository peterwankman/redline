/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifndef PARSER_INT_H_
#define PARSER_INT_H_

#include "parser.h"

struct edps_ctx_t {
	edlx_ctx_t *edlx_ctx;
	edps_instr_t *instr;
	int n_subexpr;
	const char *prompt;
};

static int ps_after_range(edps_ctx_t *ctx);
static int ps_copy(edps_ctx_t *ctx);
static int ps_move(edps_ctx_t *ctx);
static int ps_range_end(edps_ctx_t *ctx);
static int ps_range_start(edps_ctx_t *ctx);
static int ps_repeat(edps_ctx_t *ctx);
static int ps_replace(edps_ctx_t *ctx);
static int ps_search(edps_ctx_t *ctx);
static int ps_standalone_cmd(edps_ctx_t *ctx);
static int ps_statement(edps_ctx_t *ctx);
static int ps_target_range(edps_ctx_t *ctx);
static int ps_transfer(edps_ctx_t *ctx);
static int ps_write(edps_ctx_t *ctx);

#endif
