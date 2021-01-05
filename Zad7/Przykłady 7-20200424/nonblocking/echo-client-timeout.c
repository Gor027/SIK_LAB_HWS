/*
 Program uruchamiamy z dwoma parametrami: nazwa serwera i numer jego portu.
 Program spróbuje połączyć się z serwerem, po czym będzie od nas pobierał
 linie tekstu i wysyłał je do serwera. W momencie, gdy wysyłanie się nie 
 powiedzie, próbuje czytać od serwera i wypisywać na wyjście.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "err.h"

#define BUFFER_SIZE      1024

void read_something(int sock) {
  char buffer[BUFFER_SIZE];
  ssize_t len;
  static size_t total_len = 0;
  
  len = read(sock, buffer, sizeof(buffer));
  if (len < 0)
    syserr("read");
  if (len == 0)
    fatal("connection closed");

  total_len += len;
  fprintf(stderr, "Received %zd bytes (%zd in total)\n", len, total_len);
  printf("%.*s\n", (int)len, buffer);
}

int main (int argc, char *argv[]) {
  int rc;
  int sock;
  struct addrinfo addr_hints, *addr_result;
  char line[BUFFER_SIZE];
  ssize_t to_send, send_from, len_sent, total_sent;
  struct timeval timeout;      

  if (argc != 3)
    fatal("Usage: %s host port", argv[0]);

  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1)
    syserr("socket");

  /* Trzeba się dowiedzieć o adres internetowy serwera. */
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET;
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  rc =  getaddrinfo(argv[1], argv[2], &addr_hints, &addr_result);
  if (rc != 0)
    syserr("getaddrinfo: %s", gai_strerror(rc));

  /* Nawiązujemy połączenie. */
  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) == -1)
    syserr("connect");
  freeaddrinfo(addr_result);
  
  /* Ustawiamy timeout (0.5 sekundy) dla zapisu do gniazda */
  timeout.tv_sec = 0;
  timeout.tv_usec = 500000;
  if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout)) < 0)
    syserr("setsockopt failed");

  total_sent = 0;
  for (;;) {
    if (fgets(line, sizeof line, stdin) == NULL)
      break; /* koniec wejścia */
      
    to_send = strlen(line);
    send_from = 0;
    while (to_send > 0) {
      len_sent = write(sock, line + send_from, to_send);

      if (len_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        len_sent = 0; /* timeout in write */
      
      if (len_sent < 0)
        syserr("writing on stream socket");

      to_send -= len_sent;
      send_from += len_sent;
      total_sent += len_sent;

      fprintf(stderr, "Sent %zd bytes (%zd in total)\n", len_sent, total_sent);

      if (to_send > 0)
        read_something(sock);
    }
  }

  if (close(sock) < 0)
    syserr("closing stream socket");

  return 0;
}

