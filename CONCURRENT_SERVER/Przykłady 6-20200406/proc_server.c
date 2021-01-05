/*
 Program tworzy gniazdko serwera (numer przydzielany dynamicznie), drukuje
 numer przydzielonego gniazdka i oczekuje na nim na propozycje połączeń.
 Do obsługi połączeń wysługuje się podwładnymi, tworzonymi poprzez fork().
 Gdy podwładni pracują, szef odbiera kolejne telefony.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "err.h" 

#define QUEUE_LENGTH     5

int main () {
  pid_t pid, pid1;
  int sock;
  socklen_t length;
  struct sockaddr_in server;
  int msgsock;
  char buf[1024];
  ssize_t rval;

  /* Identyfikator procesu szefa */
  printf("My PID = %d\n", (int)getpid());

  /* Tworzymy gniazdko */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1) 
    syserr("opening stream socket");

  /* Przecie, że Internet */
  server.sin_family = AF_INET;
  /* Wszystkie adresy szefa (czasem wszystkie jeden) */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Poproszę jakiś numer portu */
  server.sin_port = 0;
  /* Pora na realizację zamówienia */
  if (bind(sock, (struct sockaddr *)&server, (int)sizeof(server)) == -1) 
    syserr("binding stream socket");

  /* Dowiedzmy się, jaki to port i obwieśćmy to światu */
  length = (socklen_t)sizeof(server);
  if (getsockname(sock, (struct sockaddr *)&server, &length) == -1) 
    syserr("getting socket name");

  /* Pamiętamy o zmianie kolejności oktetów */
  printf("Socket port #%d\n", (int)ntohs(server.sin_port));

  /* Otwieramy okienko i ustawiamy kolejkę petentów */
  if (listen(sock, QUEUE_LENGTH) == -1) 
    syserr("setting listen queue length");

  /* Do pracy (nieskończona pętla - never ending story) */
  for (;;) {
    msgsock = accept(sock, (struct sockaddr *)NULL, NULL);
    if (msgsock == -1)
      syserr("accept");

    /* Tworzymy nowy process */
    switch (pid = fork()) {
      
    case -1:		/* błąd */
      syserr("error in fork");
      
    case 0:		/* podwładny */
      if (close(sock) == -1) syserr("close");
      
      pid1 = getpid();
      printf("I am child process. My PID = %d\n", (int)pid1);
      
      do {
	memset(buf, 0, sizeof(buf));
	rval = read(msgsock, buf, sizeof(buf));
	if (rval < 0)
	  syserr("reading stream message");
	else if (rval == 0)
	  printf("ending connection\n");
	else
	  printf("[%d]-->%.*s\n", (int)pid1, (int)rval, buf);
      }
      while (rval > 0);
      
      if (close(msgsock) == -1) syserr("close");
      return 0;
      
    default:		/* ... a szef dalej pracuje */
      printf("I am a parent process. The value returned by "
	     "fork() = %d\n", (int)pid);
      if (close(msgsock) == -1) syserr("close");
      
    }
  }
  exit(0);  /* Czasem coś się kończy bez powodu */
}
