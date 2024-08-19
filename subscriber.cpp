/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.hpp"
#include "helpers.hpp"

void make_int(char *value, char *payload) {
	unsigned int number = ntohl(*(int *)&payload[1]);
	if (payload[0] == 0 || !number)
		sprintf(value, "%d", number);
	else
		sprintf(value, "-%d", number);
}

void make_short(char *value, char *payload) {
	unsigned short number = ntohs(*(short *)payload);
	if (number % 100 < 10)
		sprintf(value, "%d.0%d", number / 100, number % 100);
	else
		sprintf(value, "%d.%d", number / 100, number % 100);
}

void make_float(char *value, char *payload) {
	char num_value[20];

	unsigned int number = ntohl(*(int *)&payload[1]);
	signed char pow = payload[5];
	if (pow == 0) {
		sprintf(num_value, "%d", number);
	} else {
		unsigned int aux = number;
		int digits = 0;

		while (aux) {
			digits++;
			aux /= 10;
		}

		if (pow + 1 > digits)
			digits = pow + 1;

		num_value[digits + 1] = 0;
		for (int i = digits; i >= 0; i--) {
			if (pow == 0) {
				num_value[i] = '.';
				pow--;
				continue;
			}
			num_value[i] = '0' + number % 10;
			number /= 10;
			pow--;
		}
	}

	if (payload[0] == 0)
		sprintf(value, "%s", num_value);
	else
		sprintf(value, "-%s", num_value);
}

void run_client(int sockfd, char *client_id, char *ip, char *port) {
	char buf[MSG_MAXSIZE + 1];
	memset(buf, 0, MSG_MAXSIZE + 1);

	struct chat_packet sent_packet;
	struct chat_packet recv_packet;

	struct pollfd poll_fds[3];

	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;

	poll_fds[1].fd = sockfd;
	poll_fds[1].events = POLLIN;

	// Trimit mesajul de conectare la server
	sprintf(buf, "%s %s %s", client_id, ip, port);
	sent_packet.len = strlen(buf) + 1;
	strcpy(sent_packet.message, buf);

	send_all(sockfd, &sent_packet, sent_packet.len + 2);

	while (1) {

		poll(poll_fds, 2, -1);

		if (poll_fds[0].revents & POLLIN) {
			scanf("%s", buf);
			if (strncmp(buf, "exit", 4) == 0)
				return;
			if (strncmp(buf, "subscribe", 9) == 0) {
				sent_packet.len = 10;
				sprintf(sent_packet.message, "Subscribe");
				send_all(sockfd, &sent_packet, sent_packet.len + 2);

				scanf("%s", buf);
				sent_packet.len = strlen(buf) + 1;
				sprintf(sent_packet.message, "%s", buf);
				send_all(sockfd, &sent_packet, sent_packet.len + 2);

				printf("Subscribed to topic %s\n", buf);

				continue;
			}
			if (strncmp(buf, "unsubscribe", 11) == 0) {
				sent_packet.len = 12;
				sprintf(sent_packet.message, "Unsubscribe");
				send_all(sockfd, &sent_packet, sent_packet.len + 2);

				scanf("%s", buf);
				sent_packet.len = strlen(buf) + 1;
				sprintf(sent_packet.message, "%s", buf);
				send_all(sockfd, &sent_packet, sent_packet.len + 2);

				printf("Unsubscribed from topic %s\n", buf);

				continue;
			}
			printf("INVALID COMMAND!\n");
		}

		// Primim un mesaj de la server si il afisam.
		if (poll_fds[1].revents & POLLIN) {
			int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
			DIE(rc < 0, "recv");

			if (rc == 0) {
				close(sockfd);
				break;
			}

			char src[30];
			char topic[51];
			char value[1501];
			char type[12];
			value[1500] = 0;
			topic[50] = 0;

			sscanf(recv_packet.message, "%s", src);
			udp_packet *udp_pack = (udp_packet *)(recv_packet.message + 30);
			memcpy(topic, udp_pack->topic, 50);

			if (udp_pack->type == 0) {
				sprintf(type, "INT");
				make_int(value, udp_pack->payload);
			}
			if (udp_pack->type == 1) {
				sprintf(type, "SHORT_REAL");
				make_short(value, udp_pack->payload);
			}
			if (udp_pack->type == 2) {
				sprintf(type, "FLOAT");
				make_float(value, udp_pack->payload);
			}
			if (udp_pack->type == 3) {
				sprintf(type, "STRING");
				memcpy(value, udp_pack->payload, 1500);
			}
			
			printf("%s - %s - %s - %s\n", src, topic, type, value);
			continue;
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("\n Usage: %s <ID> <ip> <port>\n", argv[0]);
		return 1;
	}
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// Parsam port-ul ca un numar
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru conectarea la server
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// Completăm in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// Ne conectăm la server
	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	run_client(sockfd, argv[1], argv[2], argv[3]);

	// Inchidem conexiunea si socketul creat
	close(sockfd);

	return 0;
}
