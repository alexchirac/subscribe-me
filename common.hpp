#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1700

struct udp_packet {
	char topic[50];
	char type;
	char payload[1500];
};

struct chat_packet {
	uint16_t len;
	char message[MSG_MAXSIZE + 1];
};

#endif
