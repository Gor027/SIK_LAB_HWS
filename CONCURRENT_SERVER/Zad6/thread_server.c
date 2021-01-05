#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "err.h"

#define LINE_SIZE 100

void *handle_connection (void *s_ptr) {
  int ret, s;
  socklen_t len;
  char line[LINE_SIZE + 1], peername[LINE_SIZE + 1], peeraddr[LINE_SIZE + 1];
  struct sockaddr_in addr;

  s = *(int *)s_ptr;
  free(s_ptr);

  len = sizeof(addr);

  /* Któż to do nas dzwoni (adres)?  */
  ret = getpeername(s, (struct sockaddr *)&addr, &len);
  if (ret == -1) 
    syserr("getsockname");

  inet_ntop(AF_INET, &addr.sin_addr, peeraddr, LINE_SIZE);
  snprintf(peername, 2*LINE_SIZE, "%s:%d", peeraddr, ntohs(addr.sin_port));

  printf("%s connection open (handled by thread %lu, pid is %d)\n",
         peername, (unsigned long)pthread_self(), getpid());

  for (;;) {
    memset(line, 0, sizeof(line));
    ret = read(s, line, sizeof(line) - 1);
    if (ret == -1) 
      syserr("read");
    else if (ret == 0)
      break;
    if (line[ret - 1] != '\n')
      line[ret++] = '\n';
    printf("%s says: %.*s", peername, (int)ret, line);
  }

  printf("%s connection closed\n", peername);
  close(s);
  return 0;
}

int main () {
  int ear, rc;
  socklen_t len;
  struct sockaddr_in server;

  /* Tworzymy gniazdko */
  ear = socket(PF_INET, SOCK_STREAM, 0);
  if (ear == -1) 
    syserr("socket");

  /* Podłączamy do centrali */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = 0;
  rc = bind(ear, (struct sockaddr *)&server, sizeof(server));
  if (rc == -1) 
    syserr("bind");


  /* Każdy chce wiedzieć jaki to port */
  len = (socklen_t)sizeof(server);
  rc = getsockname(ear, (struct sockaddr *)&server, &len);
  if (rc == -1) 
    syserr("getsockname");
 
  printf("Listening at port %d\n", (int)ntohs(server.sin_port));

  rc = listen(ear, 5);
  if (rc == -1) 
    syserr("listen");
  
  /* No i do pracy */
  for (;;) {
    int msgsock;
    int *con;
    pthread_t t;

    msgsock = accept(ear, (struct sockaddr *)NULL, NULL);
    if (msgsock == -1) {
      syserr("accept");
    }

    /* Tylko dla tego wątku */
    con = malloc(sizeof(int));
    if (!con) {
      syserr("malloc");
    }
    *con = msgsock;

    rc = pthread_create(&t, 0, handle_connection, con);
    if (rc == -1) {
      syserr("pthread_create");
    }

    /* No przecież nie będę na niego czekał ... */
    rc = pthread_detach(t);
    if (rc == -1) {
      syserr("pthread_detach");
    }
  }
}

