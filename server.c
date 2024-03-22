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
#include <netinet/tcp.h>
#include <math.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32


void run_chat_multi_server(int tcp_sock, int udp_sock) {

    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_clients = 1;
    int rc;

    struct chat_packet received_packet;
    struct client *clients = (struct client*) malloc(sizeof(struct client) * 1000);

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(tcp_sock, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea read_fds
    poll_fds[0].fd = tcp_sock;
    poll_fds[0].events = POLLIN;


  while (1) {

    rc = poll(poll_fds, num_clients, -1);
    DIE(rc < 0, "poll");

    char buf[sizeof(struct chat_packet)];

    for (int i = 0; i < num_clients; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == tcp_sock) {
          // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
          // pe care serverul o accepta
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd = accept(tcp_sock, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          int rc = recv(newsockfd, buf, 10, 0);
          DIE(rc < 0, "recv");

          int found = -1;

          for(int j = 0; j < num_clients; j++) {
						if(strcmp(clients[j].id, buf) == 0) {
							found = j;
							break;
						}
          }

          if (found == -1) {
            // se adauga noul socket intors de accept() la multimea descriptorilor
            // de citire
            poll_fds[num_clients].fd = newsockfd;
            poll_fds[num_clients].events = POLLIN;
            strcpy(clients[num_clients].id, buf);
            
						clients[num_clients].socket = newsockfd;
            num_clients++;

            printf("New client %s connected from %s:%d\n", clients[num_clients - 1].id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
          } else {
            close(poll_fds[i].fd);
            printf("Client %s already connected.\n", clients[found].id);
          }

        } else {
          // s-au primit date pe unul din socketii de client,
          // asa ca serverul trebuie sa le receptioneze
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc) {
            // conexiunea s-a inchis
            struct chat_packet input = (struct chat_packet) received_packet;
            if (input.type == 'e') {
                printf("Client %s disconnected.\n", clients[num_clients].id);
                close(poll_fds[i].fd);
                break;
            }

            // se scoate din multimea de citire socketul inchis
            for (int j = i; j < num_clients - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_clients--;           
          }
          
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 2) {
      printf("\n Usage: %s <ip> <port>\n", argv[0]);
      exit(EXIT_FAILURE);
    } 

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obtinem un socket TCP pentru receptionarea conexiunilor
    int udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(udp_socket < 0 || tcp_socket < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in server_addr, udp_addr;

    /* Set port and IP that we'll be listening for, any other IP_SRC or port will be dropped */
    memset((char*)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
    // rulam de 2 ori rapid
    int enable = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // Asociem adresa serverului cu socketul creat folosind bind
    int tcp_rc = bind(tcp_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(tcp_rc < 0, "bind");

    int udp_rc = bind(udp_socket, (const struct sockaddr *)&udp_addr, sizeof(server_addr));
    DIE(udp_rc < 0, "bind");

    run_chat_multi_server(tcp_socket, udp_socket);

    // Inchidem listenfd
    close(udp_socket);
    close(tcp_socket);

    return 0;
}