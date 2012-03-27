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

#ifndef MESSAGES_H_
#define MESSAGES_H_

 enum {
	IPC_REQUEST_REPLAY = 0x01,
	IPC_REQUEST_RECORD = 0x02,
	IPC_REQUEST_SET_UI = 0x03,
	IPC_NOTIFY_SERVER_STOP = 0x04,
	IPC_NOTIFY_NEW_REPLAY = 0x05,
	IPC_CLIENT_BUSY = 0x06,
	IPC_CLIENT_OK = 0x07,
	IPC_CLIENT_CONNECT = 0x08
};

extern const char *ipc_message_format[];

#endif /* MESSAGES_H_ */
