#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1500

struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
  char type;
  char ip[16];
  uint16_t port;
  char topic[51];
  int dataType;
};

struct client {
  char id[10];
  int socket;
  char topic[51];
};

#endif