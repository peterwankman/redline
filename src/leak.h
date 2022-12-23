/*****************************************
 * SPDX-License-Identifier: GPL-2.0-only *
 *  Copyright (C) 2022  Martin Wolters   *
 *****************************************/

#ifndef LEAK_H_
#define LEAK_H_

# ifdef _WIN32
# define WIN32_LEAN_AND_MEAN

#  ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#  endif

# endif

#endif