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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../replay/replay.h"

#include "d2pointers.h"
#include "handler.h"
#include "kernel32.h"
#include "misc.h"
#include "module.h"
#include "ntdll.h"
#include "patch.h"
#include "stubs.h"
#include "types.h"
#include "ws2_32.h"

#include "../debug.h"

#include "../version.h"

#define PAGE_ALIGN(addr) (addr & ~0xfff)

#define i386_CALL 0xE8
#define i386_JUMP 0xE9
#define BYTES     0xFF

typedef struct
{
  unsigned char type;
  const char *module;
  vaddr offset;
  size_t size;
  void *func;
  unsigned char *backup;
} hook;

void __attribute__((stdcall)) mcp_send_STUB(SOCKET, const char *, int, int);
void __attribute__((stdcall)) bncs_send_STUB(SOCKET, const char *, int, int);

unsigned long game_tick = 0;

#define N_HOOKS 8

hook
    hooks[] =
      {

            /* patches for version 1.13d */
            { i386_JUMP, "D2Client.dll", 0x1D7B4, 6, draw_ingame_STUB, NULL }, // updated
	    { i386_CALL, "Fog.dll", 0x10B65, 5, mcp_send_STUB, NULL }, // updated
	    { i386_CALL, "Fog.dll", 0x1A76D, 5, bncs_send_STUB, NULL }, // updated
	    { i386_CALL, "D2Net.dll", 0x6CC5, 5, decompress_packet, NULL }, // updated
	    { i386_JUMP, "D2Client.dll", 0xD13C, 5, send_packet_STUB, NULL }, // updated

	    { BYTES, "D2Multi.dll", 0x1219C, 3, (void *) 0x90, NULL }, // updated
	    { BYTES, "D2Multi.dll", 0x11DC9, 1, (void *) 0xEB, NULL }, // updated
	    { BYTES, "D2Win.dll", 0xC847, 1, (void *) 0xEB, NULL } // updated

    };

void
draw_ingame()
{
  if (playback) {
	  draw_text(400, 540, D2FONT_WHITE, D2FONT_STANDARD, 1, "nd2tt.so v%s [REPLAY]",
      VERSION);

	  if (!game_time) game_time = 1;
	  double progress = (double) (replay_get_tick_count() - game_tick) / (double) game_time;
	  draw_progress_bar(400, 20, 280, 10, 2, progress);
  }
  else if (record) draw_text(400, 540, D2FONT_WHITE, D2FONT_STANDARD, 1, "nd2tt.so v%s [RECORD]",
      VERSION);
  else  draw_text(400, 540, D2FONT_WHITE, D2FONT_STANDARD, 1, "nd2tt.so v%s",
      VERSION);
}

void __attribute__((stdcall)) mcp_send_STUB(SOCKET s, const char  *buf, int len, int flags) {
	if (buf[2] == 0x04 && !playback && record) {
		strcpy(replay.game, (char *) &buf[5]);
	}

	if (!playback) {
		WS_send(s, buf, len, flags);
	}
}

void __attribute__((stdcall)) bncs_send_STUB(SOCKET s, const char *buf, int len, int flags) {
	if (!playback) {
		WS_send(s, buf, len, flags);
	}
}

void print_packet(BYTE *packet, DWORD len) {
	int i;
	for (i = 0; i < len; i++) {
		printf("%02X ", packet[i]);
	}
	printf("\n");
}

void __attribute__((stdcall)) on_send_packet(DWORD len, DWORD arg, BYTE *packet) {
	if (playback && packet[0] == 0x68) game_tick = replay_get_tick_count();

	if (!record || playback) return;

	switch (packet[0]) {
		case 0x6B: {
			DWORD **t = &(*p_D2CLIENT_ui_table);
			int i;
			for (i = 1; i < 38; i++) {
				ui_table_tmp[i] = (DWORD) t[i];
			}

			valid_game = 1;
			break;
		}
		
		case 0x68: {
			replay.tick = replay_get_tick_count();
			strcpy(replay.toon, (char *) &packet[21]);
			break;
		}

		case 0x06:
		case 0x07:
		case 0x09:
		case 0x0A:
		case 0x0D:
		case 0x0E:
		case 0x10:
		case 0x11: {
			// receive cast on unit
			WORD side;
			if (packet[0] < 0x0B) side = D2SKILL_LEFT;
			else side = D2SKILL_RIGHT;

			BYTE new[16] = { 0 };
			BYTE new_c[0x448];
			
			new[0] = 0x4C;
			*(DWORD *)&new[2] = (*p_D2CLIENT_player_unit)->id;
			new[6] = get_current_skill(side);
			new[8] = 0x02;
			new[9] = *(DWORD *)&packet[1];
			*(DWORD *)&new[10] = *(DWORD *)&packet[5];

			int size = FOG_compress_packet(new_c + 2, 0x448, new, 16);
			int header = create_header(size, new_c);

			RtlEnterCriticalSection(&cs);
			replay_add_packet(&replay, new_c + (2 - header), size + header);
			RtlLeaveCriticalSection(&cs);
			DEBUG_DO(printf("added fake packet %02X for %02X (%i)\n", new[0], packet[0], GetCurrentThreadId());
			print_packet(new, 16);)
			break;
		}

		case 0x05:
		case 0x08:
		case 0x0C:
		case 0x0F: {
			// reseive cast on pos
			WORD side;
			if (packet[0] < 0x0B) side = D2SKILL_LEFT;
			else side = D2SKILL_RIGHT;

			BYTE new[17] = { 0 };
			BYTE new_c[0x448];

			new[0] = 0x4D;
			*(DWORD *)&new[2] = (*p_D2CLIENT_player_unit)->id;
			new[6] = get_current_skill(side);
			new[10] = 0x02;
			*(WORD *)&new[11] = *(WORD *)&packet[1];
			*(WORD *)&new[13] = *(WORD *)&packet[3];

			int size = FOG_compress_packet(new_c + 2, 0x448, new, 17);
			int header = create_header(size, new_c);

			RtlEnterCriticalSection(&cs);
			replay_add_packet(&replay, new_c + (2 - header), size + header);
			RtlLeaveCriticalSection(&cs);
			DEBUG_DO(printf("added fake packet %02X for %02X (%i)\n", new[0], packet[0], GetCurrentThreadId());
			print_packet(new, 17);)
			break;
		}

		case 0x01:
		case 0x03: {
			// receive walk
			BYTE new[16] = { 0 };
			BYTE new_c[0x448] = { 0 };

			new[0] = 0x0F;
			*(DWORD *)&new[2] = (*p_D2CLIENT_player_unit)->id;
			new[6] = packet[0] == 0x03 ? 0x17 : 0x01;
			*(WORD *)&new[7] = *(WORD *)&packet[1];
			*(WORD *)&new[9] = *(WORD *)&packet[3];
			*(WORD *)&new[12] = (*p_D2CLIENT_player_unit)->path->x;
			*(WORD *)&new[14] = (*p_D2CLIENT_player_unit)->path->y;

			int size = FOG_compress_packet(new_c + 2, 0x446, new, 16); // 2 byte header, we might wanna limit to 0x446 instead of 0x448
			int header = create_header(size, new_c);

			RtlEnterCriticalSection(&cs);
			replay_add_packet(&replay, new_c + (2 - header), size + header);
			RtlLeaveCriticalSection(&cs);
			DEBUG_DO(printf("added fake packet %02X for %02X (%i)\n", new[0], packet[0], GetCurrentThreadId());
			print_packet(new, 16);)
			break;
		}

		case 0x02:
		case 0x04: {
			// receive walk to unit
			BYTE new[16] = { 0 };
			BYTE new_c[0x448];

			new[0] = 0x10;
			*(DWORD *)&new[2] = (*p_D2CLIENT_player_unit)->id;
			new[6] = packet[0] == 0x04 ? 0x18 : 0x00;
			new[7] = (BYTE) (*(DWORD *)&packet[1]);
			*(DWORD *)&new[8] = *(DWORD *)&packet[5];
			*(WORD *)&new[12] = (*p_D2CLIENT_player_unit)->path->x;
			*(WORD *)&new[14] = (*p_D2CLIENT_player_unit)->path->y;

			int size = FOG_compress_packet(new_c + 2, 0x448, new, 16);
			int header = create_header(size, new_c);

			RtlEnterCriticalSection(&cs);
			replay_add_packet(&replay, new_c + (2 - header), size + header);
			RtlLeaveCriticalSection(&cs);
			DEBUG_DO(printf("added fake packet %02X for %02X (%i)\n", new[0], packet[0], GetCurrentThreadId());
			print_packet(new, 16);)
			break;
		}
	}
}

static int
install_hook(const char *module, vaddr offset, vaddr func, size_t bytes,
    unsigned char inst, unsigned char **backup)
{
  void *h = LoadLibraryA(module);
  if (h == NULL)
    {
      printf("err: could not get a handle for %s\n", module);
      return 0;
    }
  vaddr addr = (vaddr) h + offset;
  unsigned char hook[bytes];
  memset(hook, 0x90, bytes);
  if (inst == i386_CALL || inst == i386_JUMP)
    {
       hook[0] = inst;
       *(vaddr *) &hook[1] = func - (addr + 5);
    }
  else
    {
       memset(hook, func & 0xFF, bytes);
    }
  DEBUG_DO(printf("installing hook at 0x%08X to func (0x%08X)\n", addr, func)); DEBUG_DO(printf("ASM: ");)
  DEBUG_DO(inst == i386_CALL ? printf("call ") : inst == i386_JUMP ? printf("jmp ") : printf("bytes ");)
  DEBUG_DO(inst == i386_CALL || inst == i386_JUMP ? printf("0x%08X\n", func - (addr + 5)) : printf("0x%02X (%i)\n", func & 0xFF, bytes);)
  DEBUG_DO(printf("enabling write access to page(s) starting from 0x%08X\n", PAGE_ALIGN(addr));)
  if (mprotect((void *) PAGE_ALIGN(addr), bytes, PROT_READ | PROT_WRITE
      | PROT_EXEC))
    {
      printf("err: could not enable write access to page(s)\n");
      return 0;
    }
  *backup = (unsigned char *) malloc(bytes);
  memcpy((void *) *backup, (void *) addr, bytes);
  DEBUG_DO(printf("saved original code\n");)
  memcpy((void *) addr, (void *) hook, bytes);
  DEBUG_DO(printf("patched to:\n");
      int i;
      for (i = 0; i < bytes; i++)
        {
          printf("0x%02X\n", hook[i]);
        })
  DEBUG_DO(printf("disabling write access to page(s) starting from 0x%08X\n", PAGE_ALIGN(addr));)
  if (mprotect((void *) PAGE_ALIGN(addr), bytes, PROT_READ | PROT_EXEC))
    {
      printf("err: could not disable write access to page(s)\n");
      return 0;
    }
  DEBUG_DO(printf("done\n");)
  return 1;
}

int
install_hooks()
{
  int i;
  for (i = 0; i < N_HOOKS; i++)
    {
      if (!install_hook(hooks[i].module, hooks[i].offset,
          (vaddr) hooks[i].func, hooks[i].size, hooks[i].type, &hooks[i].backup))
        {
          return 0;
        }
    }
  return 1;
}

static int
remove_hook(const char *module, vaddr offset, size_t bytes,
    unsigned char *backup)
{
  void *h = LoadLibraryA(module);
  if (h == NULL)
    {
      printf("err: could not get a handle for %s\n", module);
      return 0;
    }
  vaddr addr = (vaddr) h + offset;
  DEBUG_DO(printf("enabling write access to page(s) starting from 0x%08X\n", PAGE_ALIGN(addr));)
  if (mprotect((void *) PAGE_ALIGN(addr), bytes, PROT_READ | PROT_WRITE
      | PROT_EXEC))
    {
      printf("err: could not enable write access to page(s)\n");
      return 0;
    }
  memcpy((void *) addr, (void *) backup, bytes);
  DEBUG_DO(printf("restored original code:\n");
      int i;
      for (i = 0; i < bytes; i++)
        {
          printf("0x%02X\n", backup[i]);
        })
  free(backup);
  DEBUG_DO(printf("disabling write access to page(s) starting from 0x%08X\n", PAGE_ALIGN(addr));)
  if (mprotect((void *) PAGE_ALIGN(addr), bytes, PROT_READ | PROT_EXEC))
    {
      printf("err: could not disable write access to page(s)\n");
      return 0;
    }
  DEBUG_DO(printf("done\n");)
  return 1;
}

int
remove_hooks()
{
  int i;
  for (i = 0; i < N_HOOKS; i++)
    {
      if (hooks[i].backup)
        {
          if (!remove_hook(hooks[i].module, hooks[i].offset, hooks[i].size,
              hooks[i].backup))
            {
              return 0;
            }
        }
    }
  return 1;
}
