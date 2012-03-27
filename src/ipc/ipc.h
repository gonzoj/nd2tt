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

#ifndef IPC_H_
#define IPC_H_

#define IPC_MAX_CLIENTS 1

#define IPC_MAX_DATA_SIZE 2048

#define IPC_HEADER_SIZE (sizeof(unsigned char) + sizeof(unsigned int))

typedef struct ipc_message {
	unsigned char id;
	unsigned int size;
	char data[IPC_MAX_DATA_SIZE];
} ipc_message_t;

typedef int (*ipc_connection_handler_t)(int);

extern int ipc_errno;

extern const char *ipc_message_format[];

const char * ipc_strerr(int);

int ipc_connect(const char *);

int ipc_listen(const char *, int *, ipc_connection_handler_t);

int ipc_send_message(int, unsigned char, ...);

int ipc_recv_message(int, ipc_message_t *, int, ...);

#endif /* IPC_H_ */
