/*
 * Copyright (C) 2011 gonzoj
 *
 * Please check the CREDITS file for further information.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MODULE_H_
#define MODULE_H_

#include "../replay/replay.h"

#include "ntdll.h"
#include "types.h"

extern int valid_game;
extern DWORD ui_table_tmp[38];

extern replay_t replay;

extern RTL_CRITICAL_SECTION cs;

#endif /* MODULE_H_ */
