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

#pragma once

#ifndef D2POINTERS_H_
#define D2POINTERS_H_

#include "d2structs.h"
#include "types.h"

#define FUNC(r, a, m, f, p) typedef r a (*m##_##f##_t)p; extern m##_##f##_##t m##_##f;
#define VAR(t, m, v) typedef t m##_##v##_t; extern m##_##v##_t *p_##m##_##v;
#define ASM(m, a) extern DWORD m##_##a;

/* function pointers */

/* D2Client.dll */

FUNC(void, __attribute__((stdcall)), D2CLIENT, print_game_string, (ms_wchar_t *message, int color))
FUNC(void, __attribute__((fastcall)), D2CLIENT, set_ui_var, (DWORD var_no, DWORD howset, DWORD unknown))
FUNC(void, __attribute__((regparm(3))), D2CLIENT, draw_rect_frame, (void *rect))

/* D2MCPClient.dll */

FUNC(void, __attribute__((fastcall)), D2MCPCLIENT, recv, (BYTE *packet, DWORD size))

/* D2Multi.dll */

ASM(D2MULTI, join_game_1)
ASM(D2MULTI, join_game_2)

/* Fog.dll */

FUNC(DWORD, __attribute__((fastcall)), FOG, compress_packet, (BYTE *dest, DWORD dest_size, BYTE *src, DWORD src_size))
FUNC(DWORD, __attribute__((fastcall)), FOG, decompress_packet, (BYTE *dest, DWORD dest_size, BYTE *src, DWORD src_size))

/* D2Net.dll */

FUNC(void, __attribute__((stdcall)), D2NET, send_packet, (DWORD len, DWORD arg, BYTE *packet))

/* D2Gfx.dll */

FUNC(void, __attribute__((stdcall)), D2GFX, draw_line, (int x1, int y1, int x2, int y2, DWORD color, DWORD unk))
FUNC(void, __attribute__((stdcall)), D2GFX, draw_rectangle, (int x1, int y1, int x2, int y2, DWORD color, DWORD trans))

/* D2Win.dll */

FUNC(DWORD, __attribute__((fastcall)), D2WIN, set_text_size, (DWORD size))
FUNC(void, __attribute__((fastcall)), D2WIN, draw_text, (ms_wchar_t *text, int x, int y, int color, DWORD unk))
FUNC(DWORD, __attribute__((fastcall)), D2WIN, get_text_width_file_no, (ms_wchar_t *text, int *width, DWORD *file_no))

#define _D2FUNCS_START D2CLIENT_print_game_string
#define _D2FUNCS_END D2WIN_get_text_width_file_no

#undef FUNC
#undef ASM

/* variable pointers */

/* D2MCPClient.dll */

VAR(WORD, D2MCPCLIENT, request_id)

/* D2Client.dll */

VAR(unit_any *, D2CLIENT, player_unit)
VAR(DWORD *, D2CLIENT, ui_table)

/* D2Win.dll */

VAR(DWORD, D2WIN, lobby_unk)
VAR(DWORD, D2WIN, first_control)

#define _D2VARS_START p_D2MCPCLIENT_request_id
#define _D2VARS_END p_D2WIN_first_control

#undef VAR

#endif /* D2POINTERS_H_ */
