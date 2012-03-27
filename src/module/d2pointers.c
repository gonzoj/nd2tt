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

#include "d2structs.h"
#include "types.h"

enum { MODULE_D2CLIENT, MODULE_D2COMMON, MODULE_D2GFX, MODULE_D2LANG, MODULE_D2WIN, MODULE_D2NET, MODULE_D2GAME, MODULE_D2LAUNCH, MODULE_FOG, MODULE_BNCLIENT, MODULE_STORM, MODULE_D2CMP, MODULE_D2MULTI, MODULE_D2MCPCLIENT };

#define FUNC(r, a, m, f, p, o) typedef r a (*m##_##f##_t)p; m##_##f##_t m##_##f = (m##_##f##_t) (MODULE_##m | (o << 8));
#define VAR(t, m, v, o) typedef t m##_##v##_t; m##_##v##_t *p_##m##_##v = (m##_##v##_t *) (MODULE_##m | (o << 8));
#define ASM(m, a, o) DWORD m##_##a = (DWORD ) (MODULE_##m | (o << 8));

/* function pointers for version 1.13d */

/* D2Client.dll */

FUNC(void, __attribute__((stdcall)), D2CLIENT, print_game_string, (ms_wchar_t *message, int color), 0x75EB0) // updated
FUNC(void, __attribute__((fastcall)), D2CLIENT, set_ui_var, (DWORD var_no, DWORD howset, DWORD unknown), 0x1C190) // updated
FUNC(void, __attribute__((regparm(3))), D2CLIENT, draw_rect_frame, (void *rect), 0x17D10) // updated

/* D2MCPClient.dll */

FUNC(void, __attribute__((fastcall)), D2MCPCLIENT, recv, (BYTE *packet, DWORD size), 0x7590) // updated

/* D2Multi.dll */

ASM(D2MULTI, join_game_1, 0x10D90) // updated
ASM(D2MULTI, join_game_2, 0x83F0) // updated

/* Fog.dll */

FUNC(DWORD, __attribute__((fastcall)), FOG, compress_packet, (BYTE *dest, DWORD dest_size, BYTE *src, DWORD src_size), -10223) // updated
FUNC(DWORD, __attribute__((fastcall)), FOG, decompress_packet, (BYTE *dest, DWORD dest_size, BYTE *src, DWORD src_size), -10224) // updated

/* D2Net.dll */

FUNC(void, __attribute__((stdcall)), D2NET, send_packet, (DWORD len, DWORD arg, BYTE *packet), -10015) // updated

/* D2Gfx.dll */

FUNC(void, __attribute__((stdcall)), D2GFX, draw_line, (int x1, int y1, int x2, int y2, DWORD color, DWORD unk), -10013) // updated
FUNC(void, __attribute__((stdcall)), D2GFX, draw_rectangle, (int x1, int y1, int x2, int y2, DWORD color, DWORD trans), -10028) // updated

/* D2Win.dll */

FUNC(DWORD, __attribute__((fastcall)), D2WIN, set_text_size, (DWORD size), -10047) // updated
FUNC(void, __attribute__((fastcall)), D2WIN, draw_text, (ms_wchar_t *text, int x, int y, int color, DWORD unk), -10076) // updated
FUNC(DWORD, __attribute__((fastcall)), D2WIN, get_text_width_file_no, (ms_wchar_t *text, int *width, DWORD *file_no), -10179) // updated

#undef FUNC
#undef ASM

/* variable pointers for version 1.13d */

/* D2MCPClient.dll */

VAR(WORD, D2MCPCLIENT, request_id, 0xAEBC) // updated

/* D2Client.dll */

VAR(unit_any *, D2CLIENT, player_unit, 0x11D050) // from McGod's list
VAR(DWORD *, D2CLIENT, ui_table, 0x11C890) // updated

/* D2Win.dll */

VAR(DWORD, D2WIN, lobby_unk, 0x8DB28) // updated
VAR(DWORD, D2WIN, first_control, 0x8DB34) // from McGod's list

#undef VAR
