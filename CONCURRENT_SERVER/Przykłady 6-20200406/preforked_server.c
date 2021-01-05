/*
 Uwaga - to jest przykład jak NIE NALEŻY pisać serwera wieloprocesowego
 
 Program tworzy gniazdko serwera (numer przydzielany dynamicznie), drukuje
 numer przydzielonego gniazdka i oczekuje na nim na propozycje połączeń.
 Do obsługi połączeń wysługuje się podwładnymi, tworzonymi poprzez fork().
 Szef nic nie robi, a wolny podwładny odbiera kolejny telefon.
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
#include <signal.h>
#include "err.h" 

#define TRUE             1
#define FALSE            0
#define QUEUE_LENGTH     5
#define NFORKS           4

static pid_t pids[NFORKS];

/* Obsługa sygnału kończenia */
static void catch_int (int sig) {
  int i;

  /* Koniec dnia pracy */
  for (i = 0; i < NFORKS; i++)
    kill(pids[i], SIGTERM);

  /* Czekamy, aż sobie pójdą */
  while (wait(NULL) > 0);
  
  fprintf(stderr, "Signal %d catched.\n", sig);
}

static pid_t make_child (int c_sock);

int main () {
  int sock;
  socklen_t length;
  struct sockaddr_in server;
  int i;

  /* Identyfikator procesu szefa */
  printf("My PID = %d\n", (int)getpid());

  /* Tworzymy gniazdko */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    syserr("opening stream socket");

  /* Przecie, że Internet */
  server.sin_family = AF_INET;
  /* Wszystkie adresy szefa (czasem wszystkie jeden) */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Poproszę jakiś numer portu */
  server.sin_port = 0;
  /* Pora na realizację zamówienia */
  if (bind(sock, (struct sockaddr *)&server, (int)sizeof server) == -1) 
    syserr("binding stream socket");

  /* Dowiedzmy się, jaki to port i obwieśćmy to światu */
  length = (socklen_t)sizeof server;
  if (getsockname(sock, (struct sockaddr *)&server, &length) == -1) 
    syserr("getting socket name");

  /* Pamiętamy o zmianie kolejności oktetów */
  printf("Socket port #%d\n", (int)ntohs(server.sin_port));

  /* Otwieramy okienko i ustawiamy kolejkę petentów */
  if (listen(sock, QUEUE_LENGTH) == -1) 
    syserr("setting listen queue length");

  for (i = 0; i < NFORKS; i++)
    pids[i] = make_child(sock);

  struct sigaction action;
  sigset_t block_mask;
  
  sigemptyset (&block_mask);
  action.sa_handler = catch_int;
  action.sa_mask = block_mask;
  action.sa_flags = 0;
  
  if (sigaction (SIGINT, &action, 0) == -1)  
    syserr("sigaction");
  
  /* Szef wypoczywa */
  while (wait(NULL) > 0);

  exit(0);
}

pid_t make_child (int sock) {
  pid_t pid;
  int msgsock;
  ssize_t rval;
  char buf[1024];

  /* Tworzymy nowy process */
  if ((pid = fork()) > 0)
    return pid;  /* Szef wraca */
  else if (pid == -1) {
    syserr("error in fork");
    return -1;
  }

  /* A podwładny do pracy (nieskończona pętla - never ending story) */
  pid = getpid();
  printf("child %d starting\n", (int)pid);

  for (;;) {
    if ((msgsock = accept(sock, (struct sockaddr *)NULL, NULL)) == -1)
      syserr("accept");
    do {
      memset(buf, 0, sizeof buf);
      rval = read(msgsock, buf, sizeof(buf));
      if (rval == -1)
	syserr("reading stream message");
      else if (rval == 0)
	printf("ending connection\n");
      else
	printf("[%d]-->%.*s\n", (int)pid, (int)rval, buf);
    }
    while (rval > 0);

    if (close(msgsock) == -1)
      syserr("close");
    
  }
  return 1;  /* Czasem coś się kończy bez powodu */
}

