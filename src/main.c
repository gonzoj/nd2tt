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
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ipc/ipc.h"
#include "ipc/messages.h"
#include "replay/replay.h"

#include "debug.h"

#include "version.h"

#define pthread_create_detached(t, r, a) \
		pthread_attr_t attr;\
		pthread_attr_init(&attr);\
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);\
		pthread_create(&(t), &attr, (r), (void *) (a));\
		pthread_attr_destroy(&attr);

#define pthread_create_joinable(t, r, a) \
		pthread_create(&(t), NULL, (r), (void *) (a));

#define cmd(in, cmd) (strstr(in, cmd))

typedef struct ipc_client {
	int index;
	int ipc_fd;
	pid_t pid;
	pthread_mutex_t mutex; // commandline <-> module
	pthread_cond_t cond; // commandline <-> module
	pthread_mutex_t server_m; // D2GS <-> module
	pthread_cond_t server_cv; // D2GS <-> module
	int record;
} ipc_client_t;

ipc_client_t clients[IPC_MAX_CLIENTS];
pthread_mutex_t clients_m;

ipc_client_t *g_client;

replay_t replays;
pthread_mutex_t replays_m;

replay_t *g_replay;

int count;

pthread_t ipc_tid;
pthread_t server_tid;

int ipc_s;
int server_s;

int ipc_service_running = 0;
int server_thread_running =0;

ipc_client_t * ipc_client_new(ipc_client_t *client, int s, char *message) {
	client->index = -1;
	client->ipc_fd = s;
	sscanf(message, "%i", &client->pid);
	pthread_mutex_init(&client->mutex, NULL);
	pthread_cond_init(&client->cond, NULL);
	pthread_mutex_init(&client->server_m, NULL);
	pthread_cond_init(&client->server_cv, NULL);
	client->record = 0;
	return client;
}

void ipc_client_free(ipc_client_t *client) {
	client->index = -1;
	client->ipc_fd = 0;
	client->pid = 0;
	pthread_mutex_destroy(&client->mutex);
	pthread_cond_destroy(&client->cond);
	pthread_mutex_destroy(&client->server_m);
	pthread_cond_destroy(&client->server_cv);
	client->record = 0;
}

void ipc_client_free_all(ipc_client_t *client) {
	int i;
	for (i = 0; i < IPC_MAX_CLIENTS; i++) {
		if (client[i].index >= 0) {
			ipc_client_free(&client[i]);
		}
	}
}

void ipc_client_init(ipc_client_t *client) {
	int i;
	for (i = 0; i < IPC_MAX_CLIENTS; i++) {
		client[i].index = -1;
	}
}

int ipc_client_add(ipc_client_t *client, ipc_client_t *new) {
	int i;
	for (i = 0; i < IPC_MAX_CLIENTS; i++) {
		if (client[i].index < 0) {
			memcpy(&client[i], new, sizeof(ipc_client_t));
			client[i].index = i;
			return i;
		}
	}
	return -1;
}

ipc_client_t *  ipc_client_get(ipc_client_t *client, int index) {
	int i, j;
	for (i = 0, j = 0; i < IPC_MAX_CLIENTS; i++) {
		if (client[i].index >= 0) {
			if (++j == index) return &client[i];
		}
	}

	return NULL;
}

void ipc_dump_client_list(ipc_client_t *client) {
	printf("\n");

	int i, j;
	for (i = 0, j = 0; i < IPC_MAX_CLIENTS; i++) {
		if (client[i].index >= 0) {
			printf("[%i] %i (recording: %s)\n", ++j, client[i].pid, client[i].record ? "yes" : "no");
		}
	}

	if (!j) printf("(empty)\n");

	printf("\n");
}

void * ipc_worker_thread(void *arg) {
	int s = (int) arg;
	int index = -1;

	ipc_message_t m;
	while (ipc_recv_message(s, &m, 0) > 0) {
		switch (m.id) {

			case IPC_NOTIFY_NEW_REPLAY: {
				replay_t replay;

				pthread_mutex_lock(&replays_m);

				DEBUG_DO(printf("loading file %s\n", m.data);)

				if (replay_load_file(m.data, &replay)) {
					replay_add_file(&replays, &replay, &count);
					replay_free(&replay);
				}

				pthread_mutex_unlock(&replays_m);
			}
			break;

			case IPC_CLIENT_BUSY:
			break;

			case IPC_CLIENT_OK: {
				if (index >= 0) {
					DEBUG_DO(printf("received client OK\n");)
					if (!strcmp(m.data, "record")) {
						DEBUG_DO(printf("received OK record: signaling cmd thread\n");)
						pthread_mutex_lock(&clients[index].mutex);
						pthread_cond_signal(&clients[index].cond);
						pthread_mutex_unlock(&clients[index].mutex);
					} else if (!strcmp(m.data, "server stop")) {
						DEBUG_DO(printf("received OK server stop: signaling server thread\n");)
						pthread_mutex_lock(&clients[index].server_m);
						pthread_cond_signal(&clients[index].server_cv);
						pthread_mutex_unlock(&clients[index].server_m);
					} else {
						DEBUG_DO(printf("m.data: %s\n", m.data);)
					}
				}
			}
			break;

			case IPC_CLIENT_CONNECT: {
				ipc_client_t client;
				ipc_client_new(&client, s, m.data);

				pthread_mutex_lock(&clients_m);

				index = ipc_client_add(clients, &client);

				pthread_mutex_unlock(&clients_m);
			}
			break;
		}
	}

	// just to make sure
	pthread_mutex_lock(&clients[index].mutex);
	pthread_cond_signal(&clients[index].cond);
	pthread_mutex_unlock(&clients[index].mutex);

	pthread_mutex_lock(&clients[index].server_m);
	pthread_cond_signal(&clients[index].server_cv);
	pthread_mutex_unlock(&clients[index].server_m);

	pthread_mutex_lock(&clients_m);

	if (index >= 0)	ipc_client_free(&clients[index]);

	pthread_mutex_unlock(&clients_m);

	close(s);

	pthread_exit(NULL);
}

int ipc_handler(int s) {
	pthread_t tid;
	pthread_create_detached(tid, ipc_worker_thread, s);

	return 0;
}

void * ipc_service(void *unused) {
	ipc_service_running = 1;

	ipc_client_init(clients);

	ipc_listen("nd2tt_ipc", &ipc_s, ipc_handler);

	DEBUG_DO(printf("ipc service exits\n");)

	ipc_service_running = 0;

	pthread_exit(NULL);
}

unsigned char server_recv(int s) {
	unsigned char packet[1024] = { 0 };
	recv(s, packet, 1024, 0);
	return packet[0];
}

void * server_worker_thread(void *arg) {
	int s = (int) arg;

	// retrieve global variables
	ipc_client_t *client = g_client;
	replay_t *replay = g_replay;

	// signal that a new request may be made
	pthread_mutex_lock(&client->mutex);
	pthread_cond_signal(&client->cond);
	pthread_mutex_unlock(&client->mutex);

	if (!client || !replay) pthread_exit(NULL);
	
	unsigned char packet[2] = { 0xAF, 0x01 };
	send(s, packet, 2, 0);
	DEBUG_DO(printf("sent 0xAF\n");)

	if (server_recv(s) == 0x68) {
		DEBUG_DO(printf("received 0x68\n");)

		send(s, replay->head->data, replay->head->size, 0);
		DEBUG_DO(printf("sent replay->head->data\n");)

#ifdef DEBUG
		printf("received 0x%02X\n", server_recv(s));
#else
		server_recv(s);
#endif

		packet[0] = 0x02;
		packet[1] = 0x80;
		send(s, packet, 2, 0);
		DEBUG_DO(printf("sent 0x02 0x80\n");)

		packet[1] = 0x5C;
		send(s, packet, 2, 0);
		DEBUG_DO(printf("sent 0x02 0x5C\n");)
	}

	if (server_recv(s) == 0x6b) {
		DEBUG_DO(printf("received 0x6B\n");)
		unsigned long tick = replay_get_tick_count();

		DEBUG_DO(int i = 0;)
		r_packet_t *p;
		for (p = replay->head->next; p; p = p->next) {
			while (replay_get_tick_count() - tick < p->interval) usleep(1000);
			tick = replay_get_tick_count();

			if (p->size == 4 && p->data[0] == 'N' && p->data[1] == 'P') {
				DEBUG_DO(printf("sending UI packet: %i %0X\n", p->data[2], p->data[3]);)
				ipc_send_message(client->ipc_fd, IPC_REQUEST_SET_UI, p->data[2], p->data[3]);
			} else {
				send(s, p->data, p->size, 0);
				DEBUG_DO(printf("sent compressed packet %i (%li bytes)\n", ++i, p->size);)
			}
		}
	}

	if (!pthread_mutex_lock(&client->server_m)) {

		ipc_send_message(client->ipc_fd, IPC_NOTIFY_SERVER_STOP);

		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 5;
		pthread_cond_timedwait(&client->server_cv, &client->server_m, &ts); // ipc client disconnected before we locked mutex?

		pthread_mutex_unlock(&client->server_m);

	}

	close(s);
	
	pthread_exit(NULL);
}

void * server_thread(void *unused) {
	server_thread_running = 1;

	if ((server_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_DO(printf("error: failed to create socket\n");)
		server_thread_running = 0;
		pthread_exit(NULL);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(4000);

	if (bind(server_s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) {
		DEBUG_DO(printf("error: failed to bind to socket\n");)
		server_thread_running = 0;
		pthread_exit(NULL);
	}

	if (listen(server_s, IPC_MAX_CLIENTS)) {
		DEBUG_DO(printf("error: failed to listen to socket\n");)
		server_thread_running = 0;
		pthread_exit(NULL);
	}

	int s;
	struct sockaddr_in peer;
	socklen_t len = sizeof(struct sockaddr_in);
	while ((s = accept(server_s, (struct sockaddr *) &peer, &len)) >= 0) {
		pthread_t tid;
		pthread_create_detached(tid, server_worker_thread, s);
	}

	if (server_s >= 0) close(server_s);

	DEBUG_DO(printf("thread exiting, accept failed (%s)\n", strerror(errno));)

	server_thread_running = 0;

	pthread_exit(NULL);
}

char * format_replay_size(int size) {
	static char s_size[512];

	memset(s_size, 0, 512);

	int unit;
	char *s_unit;
	
	if (size / (1024 * 1024 * 1024) > 0) { // GiB
		unit = 1024 * 1024 * 1024;
		s_unit = "GiB";
	} else if (size / (1024 * 1024) > 0) { // MiB
		unit = 1024 * 1024;
		s_unit = "KiB";
	} else if (size / 1024 > 0) { // KiB
		unit = 1024;
		s_unit = "MiB";
	} else {
		unit = 1;
		s_unit = "bytes";
	}

	double s = (double) size / unit;

	snprintf(s_size, 512, "%.2f %s", s, s_unit);

	return s_size;
}

char * format_game_length(unsigned long length) {
	static char s_len[512];

	memset(s_len, 0, 512);

	length /= 1000; // convert to seconds

	int h, m, s;

	h = length / (60 * 60);
	m = (length - h * 60 * 60) / 60;
	s = length - h * 60 * 60 - m * 60;
	
	snprintf(s_len, 512, "%.2i:%.2i:%.2i", h, m, s);

	return s_len;
}

void dump_replay_file_list(replay_t *head) {
	printf("\n");

	replay_t *r;
	int i;
	for (r = head, i = 1; r && count; r = r->next, i++) {
		printf("[%i] %s (%s): %s in game %s for %s - %s\n", i, r->file.name, format_replay_size(r->file.size), r->toon, r->game, format_game_length(r->length), r->time);
	}

	if (!count) {
		printf("(empty)\n");
	}

	printf("\n");
}

replay_t * replay_get(replay_t *head, int index) {
	int i;
	replay_t *r;
	for (r = head, i = 1; r; r = r->next, i++) {
		if (i == index) {
			return r;
		}
	}

	return NULL;
}

void commandline() {

	while (1) {
		printf("\n");
		
		printf(" > ");

		char input[512];
		fgets(input, 512, stdin);

		printf("\n");
		
		if (cmd(input, "quit") || cmd(input, "q")) {
			break;
		} else if (cmd(input, "list clients") || cmd(input, "lc")) {
			pthread_mutex_lock(&clients_m);

			ipc_dump_client_list(clients);

			pthread_mutex_unlock(&clients_m);
		} else if (cmd(input, "list replays") || cmd(input, "lr")) {
			pthread_mutex_lock(&replays_m);

			dump_replay_file_list(&replays);

			pthread_mutex_unlock(&replays_m);
		} else if (cmd(input, "record") || cmd(input, "r")) {
			if (!ipc_service_running) printf("error: ipc service not running\n");

			pthread_mutex_lock(&clients_m);

			int i = -1;
			if (sscanf(input, "record %i", &i) <= 0) {
				sscanf(input, "r %i", &i);
			}

			ipc_client_t *c = ipc_client_get(clients, i);
			if (c) {
				if (pthread_mutex_lock(&c->mutex)) continue;

				c->record = !c->record;

				while (1) {

					DEBUG_DO(printf("send ipc message to [%i] %i\n", i, c->pid);)
					if (ipc_send_message(c->ipc_fd, IPC_REQUEST_RECORD, c->record) <= 0) break;

					//pthread_cond_wait(&c->cond, &c->mutex);
					struct timespec ts;
					clock_gettime(CLOCK_REALTIME, &ts);
					ts.tv_sec += 1;
					if (!pthread_cond_timedwait(&c->cond, &c->mutex, &ts)) break;

				}

				pthread_mutex_unlock(&c->mutex);
			} else {
				printf("no client [%i] found\n", i);
				printf("type 'list clients' to see available clients\n");
			}

			pthread_mutex_unlock(&clients_m);
		} else if (cmd(input, "play") || cmd(input, "p")) {
			if (!ipc_service_running) printf("error: ipc service not running\n");
			if (!server_thread_running) printf("error: D2GS not running\n");

			pthread_mutex_lock(&clients_m);
			pthread_mutex_lock(&replays_m);

			int i = -1, j = -1;
			if (sscanf(input, "play %i %i", &i, &j) <= 0) {
				sscanf(input, "p %i %i", &i, &j);
			}

			ipc_client_t *c = ipc_client_get(clients, i);
			replay_t *r = replay_get(&replays, j);

			if (c) {
				if (r) {
					if (pthread_mutex_lock(&c->mutex)) continue;

					g_client = c;
					g_replay = r;

					while (1) {

						DEBUG_DO(printf("send ipc message [%i] %s to [%i] %i\n", j, r->file.name, i, c->pid);)
						if (!server_thread_running || ipc_send_message(c->ipc_fd, IPC_REQUEST_REPLAY, r->length, r->game) <= 0) break;

						//pthread_cond_wait(&c->cond, &c->mutex);
						struct timespec ts;
						clock_gettime(CLOCK_REALTIME, &ts);
						ts.tv_sec += 1;
						if (!pthread_cond_timedwait(&c->cond, &c->mutex, &ts)) break;

					}

					pthread_mutex_unlock(&c->mutex);
				} else {
					printf("no replay [%i] found\n", j);
					printf("type 'list replays' to see loaded replay files\n");
				}
			} else {
				printf("no client [%i] found\n", i);
				printf("type 'list clients' to see available clients\n");
			}

			pthread_mutex_unlock(&replays_m);
			pthread_mutex_unlock(&clients_m);
		} else {
			printf("\n");
			printf("available commands (long/short):\n");
			printf("\n");
			printf("	quit/q:                 exit program\n");
			printf("	list clients/lc:        list all connected clients\n");
			printf("        list replays/ls:        list all loaded replay files\n");
			printf("        record 'c'/r 'c':       tell client listed as ['n'] to record gameplay\n");
			printf("        play 'c' 'r'/p 'c' 'r': tell client ['c'] to play replay file listed as ['r']\n");
			printf("\n");
		}
	}

	printf("\n");
}

int main(int argc, char **argv) {
	printf("nd2tt v%s\n\n", VERSION);

	char *home = getenv("HOME");
	if (!home) {
		printf("error: failed to retrieve home direcotry\n");
		exit(EXIT_FAILURE);
	}

	if (chdir(home)) {
		printf("error: failed to change to %s\n", home);
		exit(EXIT_FAILURE);
	}

	if (chdir(".nd2tt") && errno == ENOENT) {
		printf("%s/.nd2tt doesn't exist, creating... ", home);
		int ret = mkdir(".nd2tt", S_IRWXU) || chdir(".nd2tt");
		printf("%s\n", ret ? "failed" : "done");
		if (ret) exit(EXIT_FAILURE);
	}

	struct stat st;
	if (stat("replays", &st) && errno == ENOENT) {
		printf("replay directory doesn't exist, creating... ");
		int ret = mkdir("replays", S_IRWXU);
		printf("%s\n", ret ? "failed" : "done");
		if (ret) exit(EXIT_FAILURE);
	}

	if (chdir("replays")) {
		printf("error: failed to enter replay directory\n");
		exit(EXIT_FAILURE);
	}

	printf("loading replays from %s/.nd2tt/replays... ", home);
	replay_new(&replays);

	if (!replay_load_all(".", &replays, &count)) {
		printf("failed\n");
		exit(EXIT_FAILURE);
	} else {
		printf("\n");
		dump_replay_file_list(&replays);
	}

	if (chdir("..")) {
		printf("failed to re-enter parent directory\n");
		exit(EXIT_FAILURE);
	}

	pthread_mutex_init(&replays_m, NULL);
	pthread_mutex_init(&clients_m, NULL);

	printf("starting up ipc service... ");
	printf("%s\n", pthread_create(&ipc_tid, NULL, ipc_service, NULL) ? "failed" : "done");

	printf("starting up D2GS on localhost... ");
	printf("%s\n", pthread_create(&server_tid, NULL, server_thread, NULL) ? "failed" : "done");

	commandline();

	printf("shutting down D2GS on localhost... ");
	fflush(stdin);
	shutdown(server_s, SHUT_RDWR);
	pthread_join(server_tid, NULL);
	printf("done\n");

	printf("shutting down ipc service... ");
	fflush(stdin);
	shutdown(ipc_s, SHUT_RDWR);
	pthread_join(ipc_tid, NULL);
	printf("done\n");

	pthread_mutex_destroy(&replays_m);
	pthread_mutex_destroy(&clients_m);

	replay_free(&replays);

	printf("\nexiting\n");

	exit(EXIT_SUCCESS);
}
