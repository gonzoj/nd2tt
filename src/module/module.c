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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../ipc/ipc.h"
#include "../ipc/messages.h"
#include "../replay/replay.h"

#include "d2pointers.h"
#include "handler.h"
#include "misc.h"
#include "ntdll.h"
#include "patch.h"
#include "populate.h"

#include "../debug.h"

#define ABORT printf("\n*** FAILED TO LOAD MODULE ***\n\n"); printf("exiting...\n"); return;

int valid_game = 0;
DWORD ui_table_tmp[38];

replay_t replay;

RTL_CRITICAL_SECTION cs;

static char *replay_dir = ".";

static int loaded;

static int ipc_s;

void * module_thread(void *);
pthread_t module_tid;

void * lobby_thread(void *);
pthread_t lobby_tid;

void stop_module_threads();

void __attribute__((constructor))
_init_module()
{
  printf("\n*** INIT MODULE ***\n\n");
  printf("populating kernel32 functions... ");
  if (populate_kernel32_funcs())
    {
      printf("done\n");
    }
  else
    {
      ABORT
    }
  printf("populating ws2_32 functions... ");
  if (populate_ws2_32_funcs())
    {
      printf("done\n");
    }
  else
    {
      ABORT
    }
  printf("populating ntdll functions... ");
  if (populate_ntdll_funcs())
    {
      printf("done\n");
    }
  else
    {
      ABORT
    }
  printf("populating d2 functions... ");
  if (populate_d2funcs())
    {
      printf("done\n");
    }
  else
    {
      ABORT
    }
  printf("populating d2 variables... ");
  if (populate_d2vars())
    {
      printf("done\n");
    }
  else
    {
      ABORT
    }
  printf("installing hooks... ");
  if (install_hooks())
    {
      printf("done\n");
    }
  else
    {
      ABORT
    }
  printf("starting module threads... ");
  if (pthread_create(&module_tid, NULL, module_thread, NULL) || pthread_create(&lobby_tid, NULL, lobby_thread, NULL))
    {
      printf("err: could not create threads\n");
      ABORT
    }
  else
    {
      printf("done\n");
    }
  printf("\n*** MODULE LOADED ***\n\n");
}

void __attribute__((destructor))
_finit_module()
{
  printf("\n*** FINIT MODULE ***\n\n");
  printf("stopping module threads... ");
  stop_module_threads();
  printf("done\n");
  printf("removing hooks... ");
  if (remove_hooks())
    {
      printf("done\n");
    }
  else
    {
      printf("*** FAILED TO UNLOAD MODULE CORRECTLY ***\n\n");
      return;
    }
  printf("\n*** MODULE UNLOADED ***\n\n");
}

void * lobby_thread(void *arg) {
	loaded = 1;

	replay_new(&replay);

	while (loaded) {
		if (*p_D2WIN_lobby_unk) {
			if (record && replay.count > 0) {
				char file[512] = { 0 };

				replay_save_file(replay_dir, &replay, file);
				DEBUG_DO(printf("saved replay for game %s in file %s\n", replay.game, file);)

				if (!strcmp(replay_dir, ".")) { // resolve cwd
					char buf[512];
					if (getcwd(buf, 512)) {
						char *f = strrchr(file, '/');
						if (!f) f = file;

						strncat(buf, f, 512);

						memset(file, 0, 512);

						strcpy(file, buf);
					}
				}
				ipc_send_message(ipc_s, IPC_NOTIFY_NEW_REPLAY, file);

				replay_free(&replay);

				replay_new(&replay);
			}
			valid_game = 0;
		} else if (valid_game && record && !playback) {
			DWORD **t = &(*p_D2CLIENT_ui_table);

			int i;
			for (i = 1; i < 38; i++) {
				if (ui_table_tmp[i] != (DWORD) t[i]) {
					BYTE packet[4];
					packet[0] = 'N'; packet[1] = 'P'; packet[2] = i; packet[3] = ui_table_tmp[i] ? 0x01 : 0x00;

					replay_add_packet(&replay, packet, 4);

					DEBUG_DO(printf("added UI packet: %c %c %i %02X\n", packet[0], packet[1], packet[2], packet[3]);)

					ui_table_tmp[i] = !ui_table_tmp[i];
				}
			}
		}

		usleep(40000);
	}

	if (replay.count > 0) {
		replay_free(&replay);
	}

	DEBUG_DO(printf("lobby thread exits\n");)
	
	//pthread_exit(NULL);
	return NULL;
}

void * module_thread(void *arg) {
	loaded = 1;

	char *home = getenv("HOME");
	if (home) {
		char buf[512];
		snprintf(buf, 512, "%s/.nd2tt/replays", home);
		struct stat st;
		if (!stat(buf, &st)) {
			replay_dir = strdup(buf);
		}
	}

	RtlInitializeCriticalSection(&cs);

	while (loaded) {
		while ((ipc_s = ipc_connect("nd2tt_ipc")) < 0 && loaded) {
			sleep(1);
		}

		ipc_send_message(ipc_s, IPC_CLIENT_CONNECT, getpid());

		ipc_handler(ipc_s);

		close(ipc_s);

		sleep(1);
	}

	RtlDeleteCriticalSection(&cs);

	if (strcmp(replay_dir, ".")) free(replay_dir);

	DEBUG_DO(printf("module thread exits\n");)

	//pthread_exit(NULL);
	return NULL;
}

void stop_module_threads() {
	if (!loaded) return;

	loaded = 0;

	shutdown(ipc_s, SHUT_RDWR);
	close(ipc_s);

	pthread_join(lobby_tid, NULL);
	pthread_join(module_tid, NULL);
}
