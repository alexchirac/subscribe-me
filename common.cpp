#include "common.hpp"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
	// size_t bytes_remaining = len;
	char *buff = (char *)buffer;
	
	uint16_t bytes_remaining;
	size_t bytes_received = recv(sockfd, &bytes_remaining, 2, 0);

	while(bytes_remaining && bytes_received) {
	int read_bytes = recv(sockfd, buff + bytes_received, bytes_remaining, 0);

	if (read_bytes <= 0)
		break;

	bytes_received += read_bytes;
	bytes_remaining -= read_bytes;
	}
	
	// memcpy(buff, bytes_remaining, 2)
	return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
	size_t bytes_sent = 0;
	size_t bytes_remaining = len;
	char *buff = (char *)buffer;
	
	while(bytes_remaining) {

	int sent_bytes = send(sockfd, buff + bytes_sent, len, 0);

	if (sent_bytes <= 0)
		break;
	
	bytes_sent += sent_bytes;
	bytes_remaining -= sent_bytes;
	}
	
	return bytes_sent;
}
