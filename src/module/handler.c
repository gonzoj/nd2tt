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
#include <unistd.h>

#include "../ipc/ipc.h"
#include "../ipc/messages.h"

#include "d2pointers.h"
#include "misc.h"
#include "types.h"

#include "../debug.h"

int playback = 0;
int record = 0;

unsigned long game_time = 0;

void ipc_handler(int s) {
	int record_set = 0;
	ipc_message_t m;

	while (ipc_recv_message(s, &m, 0) > 0) {
		switch (m.id) {
			case IPC_REQUEST_REPLAY: {
				if (playback) break; // drop requests after we accepted them

				if (!*p_D2WIN_lobby_unk || !*p_D2WIN_first_control) ipc_send_message(s, IPC_CLIENT_BUSY, "in game");
				/*while (!*p_D2WIN_lobby_unk || !*p_D2WIN_first_control) {
					sleep(1);
				}*/
				else {
					char game[16] = { 0 };
					sscanf(m.data, ipc_message_format[m.id], &game_time, &game);

					// dirty fix for retrieving game names including white space
					char *ws = strchr(m.data, ' ');
					if (!ws) break;
					ws++;
					strncpy(game, ws, 16);

					playback = 1;

					sleep(1); // careful in case client was busy

					join_game(game, "", "127.0.0.1"); // if joining the game fails, nd2tt won't notice and wait for D2GS to start

					ipc_send_message(s, IPC_CLIENT_OK, "replay");

				}
			}
			break;

			case IPC_REQUEST_RECORD: {

				if (!*p_D2WIN_lobby_unk || !*p_D2WIN_first_control) ipc_send_message(s, IPC_CLIENT_BUSY, "in game");
				/*while (!*p_D2WIN_lobby_unk) {
					sleep(1);
				}*/
				else {
					int record_req;
					sscanf(m.data, ipc_message_format[m.id], &record_req);

					if (record_req == record && record_set) break; // drop requests sent after we accepted them

					else record = record_req;

					record_set = 1; // ensure that we don't drop requests from a new instance of nd2tt

					ipc_send_message(s, IPC_CLIENT_OK, "record");
				}
			}
			break;

			case IPC_REQUEST_SET_UI: { // stops working when ipc client waits for client to be ready
				if (!playback) break;

				BYTE index, value;
				//sscanf(m.data, ipc_message_format[m.id], (char *) &index, (char *) &value);
				index = m.data[0];
				value = m.data[1];

				D2CLIENT_set_ui_var(index, value, 1);

				break;
			}
			
			case IPC_NOTIFY_SERVER_STOP: {
				playback = 0;

				ipc_send_message(s, IPC_CLIENT_OK, "server stop");

				DEBUG_DO(printf("received IPC_NOTIFY_SERVER_STOP\n"));
			}
			break;

			default: {
				DEBUG_DO(printf("error: ipc: unknown message %02X\n", m.id);)
			}
			break;
		}
	}

	// after the ipc connection has been closed there is no way for nd2tt to signal the module
	// that the replay has ended, so just in case a recording was played we set playback to false
	// in order to unblock BNCS/MCP send operations
	playback = 0;

	DEBUG_DO(printf("ipc handler exits\n");)
}
