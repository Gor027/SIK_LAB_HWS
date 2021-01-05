/*
 Ten program używa poll(), aby równocześnie obsługiwać wielu klientów
 bez tworzenia procesów ani wątków.
*/

#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "err.h"

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1024

static int finish = FALSE;

int activeClients = 0;
int allClients = 0;
int serverClients = 0;

/* Obsługa sygnału kończenia */
static void catch_int(int sig) {
    finish = TRUE;
    fprintf(stderr,
            "Signal %d catched. No new connections will be accepted.\n", sig);
}

void socketByPort(struct pollfd *client, int clientType, int portType) {
    struct sockaddr_in server;
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(portType);

    client[clientType].fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client[clientType].fd == -1)
        syserr("open socket");

    if (bind(client[clientType].fd, (struct sockaddr *) &server, (socklen_t) sizeof(server)) == -1)
        syserr("bind");

    size_t len = sizeof(server);

    if (getsockname(client[clientType].fd, (struct sockaddr *) &server, (socklen_t *) &len) == -1)
        syserr("socket name");

    if (listen(client[clientType].fd, 5) == -1)
        syserr("listen");
}

void closeInterrupted(struct pollfd *client) {
    if (finish == TRUE && client[0].fd >= 0) {
        if (close(client[0].fd) < 0)
            syserr("close");

        client[0].fd = -1;
    }
}

void openConnection(struct pollfd *client, int *cliTypes, int clientType) {
    if (finish == FALSE && (client[clientType].revents & POLLIN)) {
        int msgsock = accept(client[clientType].fd, (struct sockaddr *) 0, (socklen_t *) 0);

        if (msgsock == -1) {
            syserr("accept");
        } else {
            int i;
            for (i = 2; i < _POSIX_OPEN_MAX; ++i) {
                if (client[i].fd == -1) {
                    fprintf(stderr, "Received new connection (%d)\n", i);

                    client[i].fd = msgsock;
                    client[i].events = POLLIN;
                    cliTypes[i] = clientType;
                    activeClients++;

                    if (clientType == 0) {
                        allClients++;
                        serverClients++;
                    }

                    break;
                }
            }
            if (i >= _POSIX_OPEN_MAX) {
                fprintf(stderr, "Too many clients\n");
                if (close(msgsock) < 0) {
                    syserr("close");
                }
            }
        }
    }
}

void closeConnection(struct pollfd *client, int *cliTypes, int index) {
    fprintf(stderr, "Ending connection\n");
    if (close(client[index].fd) < 0) {
        syserr("close");
    }
    client[index].fd = -1;

    if (cliTypes[index] == 0) {
        serverClients--;
    }

    cliTypes[index] = -1;
    activeClients--;
}

/* Check if just the first 5 letter match to "count" */
int ignoreNotCount(const char *count, char *buf, int countLength) {
    for (int i = 0; i < countLength; i++) {
        if (count[i] != buf[i]) {
            return 0;
        }
    }

    return 1;
}

void printAndClose(struct pollfd *client, int *cliTypes, int index, char *buf) {
    if (strlen(buf) == 7 && ignoreNotCount("count", buf, 5)) {
        char str[BUF_SIZE];
        sprintf(str, "Number of active clients: %d\nTotal number of clients: %d\n", serverClients, allClients);
        write(client[index].fd, str, strlen(str));
    }

    closeConnection(client, cliTypes, index);
}

void rcvFromClient(struct pollfd *client, int *cliTypes, int index, char *buf, ssize_t rval) {
    if (cliTypes[index] == 0) {
        printf("-->%.*s\n", (int) rval, buf);
    } else if (cliTypes[index] == 1) {
        printAndClose(client, cliTypes, index, buf);
    } else {
        fprintf(stderr, "NOT OK!!!\n");
    }
}

void serveConnection(struct pollfd *client, int *cliTypes, char *buf) {
    for (int i = 2; i < _POSIX_OPEN_MAX; ++i) {
        if (client[i].fd != -1 && (client[i].revents & (POLLIN | POLLERR))) {
            ssize_t rval = read(client[i].fd, buf, BUF_SIZE);
            if (rval < 0) {
                fprintf(stderr, "Reading message (%d, %s)\n", errno, strerror(errno));
                if (close(client[i].fd) < 0) {
                    syserr("close");
                }
                client[i].fd = -1;

                if (cliTypes[i] == 0) {
                    serverClients--;
                }

                cliTypes[i] = -1;
                activeClients--;
            } else if (rval == 0) {
                closeConnection(client, cliTypes, i);
            } else {
                rcvFromClient(client, cliTypes, i, buf, rval);
            }
        }
    }
}

void serveAllIfPositive(struct pollfd *client, int *cliTypes, char *buf) {
    openConnection(client, cliTypes, 0);
    openConnection(client, cliTypes, 1);

    serveConnection(client, cliTypes, buf);
}

void serveAll(struct pollfd *client, int *cliTypes, char *buf) {
    /* Negative means infinite waiting */
    int count = poll(client, _POSIX_OPEN_MAX, -1);

    if (count == -1) {
        if (errno == EINTR)
            fprintf(stderr, "Interrupted system call\n");
        else
            syserr("poll");
    } else if (count > 0) {
        serveAllIfPositive(client, cliTypes, buf);
    } else {
        fprintf(stderr, "Not Ok!!!\n");
    }
}

int main(int argc, char *argv[]) {
    struct pollfd client[_POSIX_OPEN_MAX];
    int cliTypes[_POSIX_OPEN_MAX];
    char buf[BUF_SIZE];
    struct sigaction action;
    sigset_t block_mask;

    if (argc != 3) {
        fatal("Usage: %s port port_kontrolny", argv[0]);
    }

    fprintf(stderr, "_POSIX_OPEN_MAX = %d\n", _POSIX_OPEN_MAX);

    /* Po Ctrl-C kończymy */
    sigemptyset(&block_mask);
    action.sa_handler = catch_int;
    action.sa_mask = block_mask;
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &action, 0) == -1)
        syserr("sigaction");

    /* Inicjujemy tablicę z gniazdkami klientów, client[0] to gniazdko centrali */
    for (int i = 0; i < _POSIX_OPEN_MAX; ++i) {
        client[i].fd = -1;
        client[i].events = POLLIN;
        client[i].revents = 0;
    }

    int port = atoi(argv[1]);
    int port_kontrolny = atoi(argv[2]);

    socketByPort(client, 0, port);
    socketByPort(client, 1, port_kontrolny);

    do {
        for (int i = 0; i < _POSIX_OPEN_MAX; ++i) {
            client[i].revents = 0;
        }

        closeInterrupted(client);
        serveAll(client, cliTypes, buf);
    } while (finish == FALSE || activeClients > 0);

    if (client[0].fd >= 0 && close(client[0].fd) < 0)
        syserr("Close Main");

    return 0;
}