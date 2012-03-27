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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "../replay/replay.h"

#include "d2pointers.h"
#include "handler.h"
#include "kernel32.h"
#include "misc.h"
#include "module.h"
#include "ntdll.h"
#include "stubs.h"

#include "../debug.h"

#define LINE(offsets, x1, y1, x2, y2, i) offsets[i++] = x1; offsets[i++] = y1; offsets[i++] = x2; offsets[i++] = y2;

typedef struct RECT {
	int left;
	int top;
	int right;
	int bottom;
} RECT;

void rect_new(RECT *rc, int l, int t, int r, int b) {
	rc->left = l;
	rc->top = t;
	rc->right = r;
	rc->bottom = b;
}

/*
 * we have to manually transform our strings because *nix
 * wchar_t is 4 bytes while windows uses 2 bytes only
 */

void
char_to_ms_wchar(char *str, ms_wchar_t *wstr)
{
  int i;
  for (i = 0; i < strlen(str); i++)
    {
      wstr[i] = (ms_wchar_t) (str[i] & 0xFF);
    }
  wstr[strlen(str)] = '\0';
}

void
ms_wchar_to_char(ms_wchar_t *wstr, char *str)
{
  int i;
  for (i = 0; wstr[i] != '\0'; i++)
    {
      str[i] = (char) (wstr[i] & 0xFF);
    }
  str[i] = '\0';
}

void
print_ingame(int color, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  char msg[1024];
  vsprintf(msg, format, args);
  ms_wchar_t wmsg[strlen(msg) + 1];
  char_to_ms_wchar(msg, wmsg);
  D2CLIENT_print_game_string(wmsg, color);
}

void
draw_text(int x, int y, int color, int font, int center,
    char *format, ...)
{
  va_list args;
  va_start(args, format);
  char text[1024];
  vsprintf(text, format, args);
  ms_wchar_t wtext[strlen(text) + 1];
  char_to_ms_wchar(text, wtext);
  POINT p =
    { x, y, };
  DWORD prev_font = D2WIN_set_text_size(font);
  if (center >= 0)
    {
      int width;
      DWORD file_no;
      D2WIN_get_text_width_file_no(wtext, &width, &file_no);
      p.x -= (width >> center);
    }
  D2WIN_draw_text(wtext, p.x, p.y, color, -1);
  D2WIN_set_text_size(prev_font);
}

void
draw_rectangle(int x, int y, int color)
{
  POINT p =
    { x, y };
  D2GFX_draw_line(p.x - 3, p.y + 3, p.x + 3, p.y + 3, color, -1);
  D2GFX_draw_line(p.x - 3, p.y + 3, p.x - 3, p.y - 3, color, -1);
  D2GFX_draw_line(p.x - 3, p.y - 3, p.x + 3, p.y - 3, color, -1);
  D2GFX_draw_line(p.x + 3, p.y - 3, p.x + 3, p.y + 3, color, -1);
}

void
draw_cross(int x, int y, int color)
{
  POINT p =
    { x, y };
  int lines[48];
  int i = 0;
  LINE(lines, p.x, p.y - 2, p.x + 4, p.y - 4, i)
  LINE(lines, p.x + 4, p.y - 4, p.x + 8, p.y - 2, i)
  LINE(lines, p.x + 8, p.y - 2, p.x + 4, p.y , i)
  LINE(lines, p.x + 4, p.y, p.x + 8, p.y + 2, i)
  LINE(lines, p.x + 8, p.y + 2, p.x + 4, p.y + 4, i)
  LINE(lines, p.x + 4, p.y + 4, p.x, p.y + 2, i)
  LINE(lines, p.x, p.y + 2, p.x - 4, p.y + 4, i)
  LINE(lines, p.x - 4, p.y + 4, p.x - 8, p.y + 2, i)
  LINE(lines, p.x - 8, p.y + 2, p.x - 4, p.y , i)
  LINE(lines, p.x - 4, p.y, p.x - 8, p.y - 2, i)
  LINE(lines, p.x - 8, p.y - 2, p.x - 4, p.y - 4, i)
  LINE(lines, p.x - 4, p.y - 4, p.x, p.y - 2, i)
  int j;
  for (j = 0; j < i; j += 4)
    {
      D2GFX_draw_line(lines[j], lines[j + 1], lines[j + 2], lines[j + 3],
          color, -1);
    }
}

void
draw_box(int x, int y, int width, int height, int color, int trans)
{
  POINT p =
    { x, y };
  D2GFX_draw_rectangle(p.x, p.y, p.x + width, p.y + height, color, trans);
}

void draw_progress_bar(int x, int y, int width, int height, int trans, double progress) {
	RECT r;
	rect_new(&r, x - (width / 2), y - (height / 2), x + (width / 2), y + (height / 2));
	
	D2GFX_draw_rectangle(r.left, r.top, r.right, r.bottom, D2GFX_BLACK, 2);

	int offset = width * progress;
	if (offset > width) offset = width;

	D2GFX_draw_rectangle(r.left, r.top, r.left + offset, r.bottom, D2GFX_RED, 2);

	D2CLIENT_draw_rect_frame(&r);
}

void join_game(char *name, char *password, char *addr) {
	DEBUG_DO(printf("forcing client to connect to game %s on %s\n", name, addr);)

	join_game_STUB(name, password);

	BYTE packet[13] = { 0 };
	packet[0] = 0x04;
	*(WORD *)&packet[1] = *p_D2MCPCLIENT_request_id;
	*(DWORD *)&packet[7] = inet_addr(addr);

	D2MCPCLIENT_recv(packet, 13);
}

DWORD __attribute__((fastcall)) decompress_packet(BYTE *dest, DWORD dest_size, BYTE *src, DWORD src_size) {
	int header = src_size > 0xEF ? 2 : 1;
	
	BYTE packet[src_size + header];
	memcpy(packet, src - header, src_size + header);

	DWORD ret = FOG_decompress_packet(dest, dest_size, src, src_size);

	if (dest[0] != 0xAE && dest[0] != 0xAF && dest[0] != 0x00 && dest[0] != 0x02 && dest[0] != 0x8F && dest[0] != 0xB0 && dest[0] != 0x05 && dest[0] != 0x06) {
		if (record && !playback) {
			DEBUG_DO(printf("logging new incoming packet (%i)\n", GetCurrentThreadId());)
			RtlEnterCriticalSection(&cs);
			replay_add_packet(&replay, packet, src_size + header);
			RtlLeaveCriticalSection(&cs);
		}
	}

	DEBUG_DO(if (!ret) printf("error: failed to decompress incoming packet\n");)

	return ret;
}

int create_header(int size, BYTE *output) {
	if (size > 238) {
		size += 2;
		size |= 0xF000;
		*output++ = (BYTE) (size >> 8);
		*output = (BYTE) (size & 0xFF);
		return 2;
	} else {
		*++output = (BYTE) (size + 1);
		return 1;
	}
}

WORD get_current_skill(int side) {
	if (side == D2SKILL_LEFT) {
		return *(*p_D2CLIENT_player_unit)->skill_info->left->id;
	} else {
		return *(*p_D2CLIENT_player_unit)->skill_info->right->id;
	}
}
