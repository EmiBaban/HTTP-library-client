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
#include <netinet/tcp.h>

#include "common.h"
#include "helpers.h"

void run_client(int sockfd) {
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct chat_packet sent_packet;
  struct chat_packet recv_packet;

  struct pollfd poll_fds[2];
  int npoll_fds = 2;

  // Adaugam descriptorul pentru citirea de la tastatura in lista de descriptori.
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  // Adaugam descriptorul pentru citirea de la server in lista de descriptori.
  poll_fds[1].fd = sockfd;
  poll_fds[1].events = POLLIN;

  while (1) {
    int rc = poll(poll_fds, npoll_fds, -1); // Asteptam evenimente indefinit.
    DIE(rc < 0, "poll");

    // Verificam daca am primit date de la tastatura.
    if (poll_fds[0].revents & POLLIN) {
      if (fgets(buf, sizeof(buf), stdin) && !isspace(buf[0])) {
        sent_packet.len = strlen(buf) + 1;
        
        if (strncmp(buf, "exit", 4) == 0) {
          strcpy(sent_packet.message, buf);
          sent_packet.type = 'e';
          send_all(sockfd, &sent_packet, sizeof(sent_packet));
          break;
        }
        if (strncmp(buf, "subscribe", 9) == 0) {
          char topic[51];
          int dataType;
          sent_packet.type = 's';

          int n = sscanf(buf, "subscribe %s %d", topic, &dataType);
          if (n != 2) {
              printf("Invalid subscription message.\n");
              return;
          }
          strcpy(sent_packet.topic, topic);
          sent_packet.dataType = dataType;

          // Send pack over network
          send_all(sockfd, &sent_packet, sizeof(sent_packet));
          printf("Subscribed to topic.\n");
        }

        if (strncmp(buf, "unsubscribe", 11) == 0) {
          char topic[51];
          sent_packet.type = 'u';

          int n = sscanf(buf, "unsubscribe %s", topic);
          if (n != 1) {
              printf("Invalid subscription message.\n");
              return;
          }
          strcpy(sent_packet.topic, topic);
          send_all(sockfd, &sent_packet, sizeof(sent_packet));
          printf("Unsubscribed from topic.\n");
        }

      }
    }

    // Verificam daca am primit date de la server.
    if (poll_fds[1].revents & POLLIN) { 
      rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
      if (rc == 0) {
        break;
      }
    }
  }
}


int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  if (argc != 4) {
    printf("\n Usage: %s <ID_CLIENT> <ip> <port>\n", argv[0]);
    return 1;
  }

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtinem un socket TCP pentru conectarea la server
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Ne conectăm la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  int ret = send(sockfd, argv[1], 10, 0);
  DIE(ret < 0, "send");

  int enable = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &enable, sizeof(int));

  run_client(sockfd);

  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}
