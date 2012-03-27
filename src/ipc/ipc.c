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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "ipc.h"

#ifndef UNIX_PATH_MAX
// struct sockaddr_un sizecheck;
// #define UNIX_PATH_MAX sizeof(sizecheck.sun_path)
#define UNIX_PATH_MAX 108
#endif

int ipc_errno = 0;

const char *ipc_err_s[] = {
	"",
	"error: ipc: failed to create socket",
	"error: ipc: failed to connect socket",
	"error: ipc: failed to bind socket",
	"error: ipc: failed to listen on socket",
	"error: ipc: failed to accept connection on socket"
};

const char * ipc_strerr(int i) {
	return ipc_err_s[i];
}

int ipc_connect(const char *uds) {
	int s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		ipc_errno = 0;
		return s;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, UNIX_PATH_MAX, "%c%s", '\0', uds); // abstract namespace

	if (connect(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_un))) {
		ipc_errno = 2;
		close(s); // not sure if whether to close here or not
		return -1;
	}

	return s;
}

int ipc_listen(const char *uds, int *s, int (*connection_handler)(int)) {
	*s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*s < 0) {
		ipc_errno = 1;
		return *s;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, UNIX_PATH_MAX, "%c%s", '\0', uds);

	if (bind(*s, (struct sockaddr *) &addr, sizeof(struct sockaddr_un))) {
		ipc_errno = 3;
		close(*s);
		return -1;
	}

	if (listen(*s, IPC_MAX_CLIENTS)) {
		ipc_errno = 4;
		close(*s);
		return -1;
	}

	int s_ipc;
	socklen_t addrlen = sizeof(struct sockaddr_un);;
	while ((s_ipc = accept(*s, (struct sockaddr *) &addr, &addrlen)) >= 0) {
		connection_handler(s_ipc);
	}

	if (*s >= 0) {
		if (s_ipc < 0) {
			ipc_errno = 5;
		}
		close(*s);
	}

	return 0;
}

int ipc_send(int s, unsigned char *buf, int len) {
	int total, tmp;

	for (total = 0; total < len; total += tmp) {
		tmp = send(s, buf + total, len - total, 0);

		if (tmp <= 0) {
			return tmp;
		}
	}

	return total;
}

int ipc_recv(int s, unsigned char *buf, int len) {
	int  total, tmp;

	for (total = 0; total < len; total += tmp) {
		tmp = recv(s, buf + total, len - total, 0);

		if (tmp <= 0) {
			return tmp;
		}
	}

	return total;
}

int ipc_send_message(int s, unsigned char id, ...) {
	va_list args;
	va_start(args, id);

	unsigned int size;
	char data[IPC_MAX_DATA_SIZE];
	if ((size = vsnprintf(data, IPC_MAX_DATA_SIZE, ipc_message_format[id], args)) < 0) {
		va_end(args);

		return -1;
	}
	size++; // include trailing 0

	va_end(args);

	//data[IPC_MAX_DATA_SIZE - 1] = '\0';

	unsigned char buf[IPC_HEADER_SIZE + size];
	
	buf[0] = id;
	*(unsigned int *)&buf[1] = size;
	memcpy(buf + IPC_HEADER_SIZE, data, size);

	return ipc_send(s, buf, size + IPC_HEADER_SIZE);
}

void ipc_hide_null(char *data, unsigned int size, int *mask) {
	int i;
	for (i = 0; i < size - 1; i++) {
		if (data[0] == 0) {
			data[i] = 1;
			mask[i] = 1;
		}
	}
}

void ipc_show_null(char *data, unsigned int size, int *mask) {
	int i;
	for (i = 0; i < size - 1; i++) {
		if (data[0] == 0 && mask[i]) {
			data[i] = 0;
		}
	}
}

int ipc_recv_message(int s, ipc_message_t *m, int parse, ...) {
	int r;

	r = ipc_recv(s, &m->id, sizeof(unsigned char));
	if (r <= 0) {
		return r;
	}

	r = ipc_recv(s, (unsigned char *) &m->size, sizeof(unsigned int));
	if (r <= 0) {
		return r;
	}

	r = ipc_recv(s, (unsigned char *) &m->data, m->size);
	if (m->size && r <= 0) {
		return r;
	}

	if (parse) {
		int mask[m->size];
		memset(mask, 0, m->size);
		ipc_hide_null(m->data, m->size, mask);

		va_list args;
		va_start(args, parse);

		if (vsscanf(m->data, ipc_message_format[m->id], args) == EOF) {
			va_end(args);

			return -1;
		}

		va_end(args);

		ipc_show_null(m->data, m->size, mask);
	}

	return IPC_HEADER_SIZE  + m->size;
}
