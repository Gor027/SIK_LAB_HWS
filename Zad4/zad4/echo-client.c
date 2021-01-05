#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "err.h"


/**************************************************EXPLANATION******************************************/
/* BUFFER_SIZE is changed to different values to check the output... */
/* Packet transporting checked with WireShark and large size of buffers fit in one packet when on loopback with
 * MTU is over 65000, however, when sending is being done with MTU set to 1500 for example, the packets are divided on
 * more than 30 small packets. */

#define BUFFER_SIZE 10000

int main(int argc, char *argv[]) {
  int sock;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  int i, flags, sflags;
  char buffer[BUFFER_SIZE];
  size_t len;
  ssize_t snd_len, rcv_len;
  struct sockaddr_in my_address;
  struct sockaddr_in srvr_address;
  socklen_t rcva_len;

  if (argc < 5) {
    fatal("Usage: %s host port message ...\n", argv[0]);
  }

  // 'converting' host/port in string to struct addrinfo
  (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_DGRAM;
  addr_hints.ai_protocol = IPPROTO_UDP;
  addr_hints.ai_flags = 0;
  addr_hints.ai_addrlen = 0;
  addr_hints.ai_addr = NULL;
  addr_hints.ai_canonname = NULL;
  addr_hints.ai_next = NULL;
  if (getaddrinfo(argv[1], NULL, &addr_hints, &addr_result) != 0) {
    syserr("getaddrinfo");
  }

  my_address.sin_family = AF_INET; // IPv4
  my_address.sin_addr.s_addr =
      ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
  my_address.sin_port = htons((uint16_t) atoi(argv[2])); // port from the command line

  freeaddrinfo(addr_result);

  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    syserr("socket");

  /* Arguments may be set 0 if non-valid integer is passed. */
  int n = atoi(argv[3]);
  int k = atoi(argv[4]);

  for (i = 0; i < n; i++) {
//    len = strnlen(argv[i], BUFFER_SIZE);
    len = k;
    if (len == BUFFER_SIZE) {
      (void) fprintf(stderr, "ignoring long parameter %d\n", i);
      continue;
    }
    (void) printf("sending to socket: %d bytes\n", k);
    sflags = 0;
    rcva_len = (socklen_t) sizeof(my_address);

    char toBeSent[k];
    memset(toBeSent, 0, sizeof(toBeSent));
    snd_len = sendto(sock, toBeSent, len, sflags,
        (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != (ssize_t) len) {
      syserr("partial / failed write");
    }

    (void) memset(buffer, 0, sizeof(buffer));
    flags = 0;
    len = (size_t) sizeof(buffer) - 1;
    rcva_len = (socklen_t) sizeof(srvr_address);
    rcv_len = recvfrom(sock, buffer, len, flags,
        (struct sockaddr *) &srvr_address, &rcva_len);

    if (rcv_len < 0) {
      syserr("read");
    }
    (void) printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);
  }

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  }

  return 0;
}
