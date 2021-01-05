//
// Created by gor027 on 08.04.2020.
//

#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <pthread.h>
#include "err.h"
#include <string.h>
#include <zconf.h>

#define SIZE 65
#define LINE_SIZE 1024

int reading(int s, char *buffer, size_t bufSize, const char *error) {
    memset(buffer, 0, LINE_SIZE);

    int result = read(s, buffer, bufSize);

    if (result < 0) {
        syserr(error);
    }

    return result;
}

uint64_t getSize(int s, char *str) {
    char *endPtr; // For strtoull in the end

    if (reading(s, str, SIZE, "Reading the file name size...") == 0) {
        syserr("Reading the file name size...");
    }

    /* strtoull converts in base 10 */
    return strtoull(str, &endPtr, 10);
}

void *handle_connection(void *s_ptr) {
    int ret, s;
    socklen_t len;
    struct sockaddr_in addr;
    char line[LINE_SIZE + 1], peername[LINE_SIZE + 1], peeraddr[LINE_SIZE + 1],
            fileName[LINE_SIZE + 1];

    s = *(int *) s_ptr;
    free(s_ptr);

    len = sizeof(addr);

    /* Któż to do nas dzwoni (adres)?  */
    ret = getpeername(s, (struct sockaddr *) &addr, &len);
    if (ret == -1) {
        syserr("getsockname");
    }

    inet_ntop(AF_INET, &addr.sin_addr, peeraddr, LINE_SIZE);
    snprintf(peername, 2 * LINE_SIZE, "%s:%d", peeraddr, ntohs(addr.sin_port));

    /* First get the length of the file name */
    size_t fileNameSize = getSize(s, line);

    /* Get the name of the file */
    if (reading(s, line, fileNameSize, "Reading the file name...") != fileNameSize) {
        syserr("Reading the file name....");
    }

    strcpy(fileName, line);

    /* Get the size of the file */
    uint64_t fileSize = getSize(s, line);

    printf("new client %s size=%lu file=%s\n", peername, fileSize, fileName);

    /* Get the file */
    FILE *ftPtr = fopen(fileName, "w");
    if (!ftPtr)
        system("File opening...");

    /* Sleep for 1 second */
    sleep(1);

    uint64_t totalSize = 0;

    while (1) {
        ret = reading(s, line, LINE_SIZE, "Reading line by line ....");

        if (ret == 0)
            break;

        fprintf(ftPtr, "%s", line);
        totalSize += ret;
    }

    printf("client %s has sent its file of size=%lu\n", peername, fileSize);
    printf("total size of uploaded files %lu\n", totalSize);

    close(s);
    fclose(ftPtr);

    return 0;
}

int main(int argc, char *argv[]) {
    int rc;
    socklen_t len;
    struct sockaddr_in server;

    if (argc != 2) {
        fatal("Usage: %s port", argv[0]);
    }

    int port = atoi(argv[1]);

    int ear = socket(PF_INET, SOCK_STREAM, 0);
    if (ear == -1) {
        syserr("socket");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    rc = bind(ear, (struct sockaddr *) &server, sizeof(server));

    if (rc == -1)
        syserr("bind");


    /* Każdy chce wiedzieć jaki to port */
    len = (socklen_t) sizeof(server);
    rc = getsockname(ear, (struct sockaddr *) &server, &len);
    if (rc == -1)
        syserr("getsockname");

    printf("Listening at port %d\n", (int) ntohs(server.sin_port));

    rc = listen(ear, 5);
    if (rc == -1) {
        syserr("listen");
    }

    /* No i do pracy */
    for (;;) {
        int msgsock;
        int *con;
        pthread_t t;

        msgsock = accept(ear, (struct sockaddr *) NULL, NULL);
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