/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <cstring>

#include "common.hpp"
#include "helpers.hpp"

#define TCP_NODELAY 1
#define MAX_CONNECTIONS 32

struct subscriber {
  char id[11];
  int status;
  std::vector<std::vector<char *>> topics;
};

/* Frees a topic vector */
void free_topic(std::vector<char*> &topic) {
  for (auto part : topic)
    free(part);
}

/* Frees a a subscriber's topics */
void free_sub(subscriber &sub) {
  for (auto topic : sub.topics) {
    free_topic(topic);
  }
}

/* Splits a topic in multiple tokens */
void split_topic(std::vector<char*> &split_topic, char *topic) {
  char *tok;

  tok = strtok(topic, "/");
  char *new_string = strdup(tok);
  split_topic.push_back(new_string);

  while (1)
  {
    tok = strtok(NULL, "/");
    if (tok == NULL)
      return;
    new_string = strdup(tok);
    split_topic.push_back(new_string);
  }
}

/* Checks if a 2 topics are identical */
int matches_exact_topic(std::vector<char*> topic1, std::vector<char*> topic2) {
  if (topic1.size() != topic2.size())
    return 0;

  for (long unsigned int i = 0; i < topic1.size(); i++) {
    if (strcmp(topic1[i], topic2[i]))
      return 0;
  }

  return 1;
}

/* Checks if a topic matches another one with wildcards */
int matches_topic(std::vector<char*> &checked_topic, std::vector<char*> &subbed_topic) {
  long unsigned int i = 0, j = 0;
  while (i < checked_topic.size() && j < subbed_topic.size()) {
    if (!strcmp(subbed_topic[j], "+")) {
      i++;
      j++;
      continue;
    }

    if (!strcmp(subbed_topic[j], "*")) {
      if (j == subbed_topic.size() - 1)
        return 1;

      j++;
      while (strcmp(checked_topic[i], subbed_topic[j]))
      {
        i++;
        if (i == checked_topic.size())
          return 0;
      }
      i++;
      j++;
      continue;
    }
    if (strcmp(checked_topic[i], subbed_topic[j]))
      return 0;
    
    i++;
    j++;
  }
  if (i < checked_topic.size() || j < subbed_topic.size())
    return 0;

  return 1;
}

/* Checks if a client is subscribed to a topic */
int check_sub(std::vector<char*> &checked_topic, subscriber &sub) {
  for (auto topic : sub.topics) {
    if (matches_topic(checked_topic, topic))
      return 1;
  }
  return 0;
}

/* Receives an UDP message and sends it to the clients subscribed to it's topic */
void recv_and_send_udp(int udp_fd, std::vector<struct pollfd> poll_fds, std::vector<subscriber> subscribers) {
  struct chat_packet sent_packet;
  char buf[1600];
  int rc;

  struct sockaddr_in from;
  socklen_t len = sizeof(from);
    
  int read_bytes = recvfrom(udp_fd, buf, 1600, 0, (struct sockaddr*)&from, &len);

  buf[read_bytes] = 0;
  udp_packet *udp_pack = (udp_packet *)buf;
  sent_packet.len = 30 + read_bytes + 1;
  sprintf(sent_packet.message, "%s:%hu", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
  memcpy(sent_packet.message + 30, udp_pack, sizeof(udp_packet));

  char topic[51];
  topic[50] = 0;
  memcpy(topic, udp_pack->topic, 50);  
  std::vector<char*> checked_topic;
  split_topic(checked_topic, topic);

  for (long unsigned int i = 0; i < subscribers.size(); i++) {
    if (check_sub(checked_topic, subscribers[i])) {
      rc = send_all(poll_fds[i + 3].fd, &sent_packet, sent_packet.len + 2);
      DIE(rc < 0, "send");
    }
  }
  free_topic(checked_topic);
}

/* Returns the index of a subscriber with the given id */
int sub_idx(std::vector<subscriber> subscribers, char new_id[]) {
  for (long unsigned int i = 0; i < subscribers.size(); i++) {
    if (!strcmp(subscribers[i].id, new_id)) {
      return i;
    }
  }
  
  return -1;
}

/* Processes a keyboard input */
int keyboard_input(std::vector<struct pollfd> &poll_fds, std::vector<subscriber> &subscribers) {
  char buf[BUFSIZ];
  scanf("%s", buf);
  if (!strncmp(buf, "exit", 4)) {
    for (long unsigned int i = 0; i < poll_fds.size(); i++) {
      close(poll_fds[i].fd);
    }
    for (long unsigned int i = 0; i < subscribers.size(); i++) {
      free_sub(subscribers[i]);
    }
    return 1;
  }
  printf("Invalid command!\n");
  return 0;
}

/* Processes a new_connection */
void new_connection(int listenfd, std::vector<struct pollfd> &poll_fds, std::vector<subscriber> &subscribers) {
  int rc;
  struct chat_packet received_packet;

  // Am primit o cerere de conexiune pe socketul de listen, pe care
  // o acceptam
  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  const int newsockfd =
      accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
  DIE(newsockfd < 0, "accept");

  // Dezactivez algoritmul lui NAGLE
  int flag = 1;
  rc = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
  DIE(rc < 0, "set sock opt");

  // Primesc si prelucrez de la client mesajul de conectare
  rc = recv_all(newsockfd, &received_packet,
                    sizeof(received_packet));
  DIE(rc < 0, "recv");

  char client_ID[11];
  char ip[20];
  int port;

  sscanf(received_packet.message, "%s %s %d", client_ID, ip, &port);
  int idx = sub_idx(subscribers, client_ID);

  // If the client does't exist yet
  if (idx < 0) {
    printf("New client %s connected from %s:%d.\n", client_ID, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    subscriber new_sub;
    strcpy(new_sub.id, client_ID);
    new_sub.status = 1;

    struct pollfd new_pollfd;
    new_pollfd.fd = newsockfd;
    new_pollfd.events = POLLIN;

    subscribers.push_back(new_sub);
    poll_fds.push_back(new_pollfd);
  } else {

    if (subscribers[idx].status == 0) {
      // If the client is offline, make it go online
      printf("New client %s connected from %s:%d.\n", client_ID, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
      subscribers[idx].status = 1;
      poll_fds[idx + 3].fd = newsockfd;
    } else {
      // If the client is already online
      printf("Client %s already connected.\n", client_ID);
      close(newsockfd);
    }

  }
}

/* Processes a message from a tcp client */
void receive_tcp_msg(int idx, std::vector<struct pollfd> &poll_fds, std::vector<subscriber> &subscribers) {
  int rc;
  struct chat_packet received_packet;

  // Am primit date pe unul din socketii de client, asa ca le receptionam
  rc = recv_all(poll_fds[idx].fd, &received_packet,
                    sizeof(received_packet));
  DIE(rc < 0, "recv");

  // If no bytes were received it means that the client disconnected
  if (rc == 0) {
    printf("Client %s disconnected.\n", subscribers[idx - 3].id);
    close(poll_fds[idx].fd);
    subscribers[idx - 3].status = 0; 
  } else {
    // If the client wants to subscribe to a topic 
    if (!strncmp(received_packet.message, "Subscribe", 9)) {
      rc = recv_all(poll_fds[idx].fd, &received_packet,
                    sizeof(received_packet));
      DIE(rc < 0, "recv");

      std::vector<char *> new_topic;
      split_topic(new_topic, received_packet.message);
              
      subscribers[idx - 3].topics.push_back(new_topic);
      return;
    }

    // If the client wants to unsubscribe fom a topic
    if (!strncmp(received_packet.message, "Unsubscribe", 11)) {
      rc = recv_all(poll_fds[idx].fd, &received_packet,
                            sizeof(received_packet));
      DIE(rc < 0, "recv");

      std::vector<char *> new_topic;
      split_topic(new_topic, received_packet.message);
              
      for (long unsigned int j = 0; j < subscribers[idx - 3].topics.size(); j++) {
        if (matches_exact_topic(subscribers[idx - 3].topics[j], new_topic)) {
          free_topic(subscribers[idx - 3].topics[j]);
          subscribers[idx - 3].topics.erase(subscribers[idx - 3].topics.begin() + j);
          break;
        }
      }
      free_topic(new_topic);
      return;
    }
  }
}

void run_chat_multi_server(int listenfd, int udp_fd) {

  std::vector<struct pollfd> poll_fds(3);
  std::vector<subscriber> subscribers;
  int rc;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
  // multimea poll_fds
  poll_fds[0].fd = listenfd;
  poll_fds[0].events = POLLIN;

  // Adaugam file descriptorul pentru primirea mesajelor de la clientii UDP
  poll_fds[1].fd = udp_fd;
  poll_fds[1].events = POLLIN;

  // Adaugam file descriptorul pentru citirea de la tastatura
  poll_fds[2].fd = STDIN_FILENO;
  poll_fds[2].events = POLLIN;

  while (1) {
    // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
    rc = poll(&poll_fds[0], poll_fds.size(), -1);
    DIE(rc < 0, "poll");

    for (long unsigned int i = 0; i < poll_fds.size(); i++) {
      if (poll_fds[i].revents & POLLIN) {
        // Am primit o comanda de la tastatura
        if (poll_fds[i].fd == STDIN_FILENO) {
          int ret = keyboard_input(poll_fds, subscribers);
          if (ret)
            return;
        }

        // S-a realizat o conexiune noua
        if (poll_fds[i].fd == listenfd) {
          new_connection(listenfd, poll_fds, subscribers);
          continue;
        } 

        // Am primit un mesaj de la un client TCP
        if (poll_fds[i].fd != listenfd && poll_fds[i].fd != udp_fd && poll_fds[i].fd != STDIN_FILENO) {
          receive_tcp_msg(i, poll_fds, subscribers);
          continue;
        }

        // Am primit un mesaj de la un client UDP
        recv_and_send_udp(udp_fd, poll_fds, subscribers);

      }
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("\n Usage: %s <port>\n", argv[0]);
    return 1;
  }
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listenfd < 0, "socket");

  const int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_fd < 0, "udp_socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  // rulam de 2 ori rapid
  // Vezi https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
  const int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Asociem adresa serverului cu socketul creat folosind bind
  rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind");

  rc = bind(udp_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind udp");

  run_chat_multi_server(listenfd, udp_fd);

  // Inchidem listenfd
  close(listenfd);

  return 0;
}
