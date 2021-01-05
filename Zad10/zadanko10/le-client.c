#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "err.h"

#define BUF_SIZE 1024

struct event_base *base;
struct bufferevent *bev;
char *fileName = NULL;

void writeContent(const char *buffer, const size_t buffer_size,
                  const char *error_mess) {
    if (bufferevent_write(bev, buffer, buffer_size) == -1)
        syserr(error_mess);
}

void writeSize(uint64_t number) {
    const size_t int_buffer_size = 65;
    char number_str[int_buffer_size];
    memset(number_str, 0, int_buffer_size);

    snprintf(number_str, int_buffer_size, "%064lu", number);
    writeContent(number_str, int_buffer_size, "get File_Name_Size");
}

int64_t getFileSize(FILE *file) {
    fseek(file, 0L, SEEK_END);
    int64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    return file_size;
}

void stdin_cb(evutil_socket_t descriptor, short ev, void *arg) {
    char buf[BUF_SIZE + 1];

    size_t fileNameSize = strlen(fileName);

    FILE *fPtr = fopen(fileName, "r");
    if (fPtr == NULL) {
        syserr("file open");
    }

    writeSize(fileNameSize);
    writeContent(fileName, fileNameSize, "fileName send");

    uint64_t fileSize = getFileSize(fPtr);
    writeSize(fileSize);

    for (uint64_t i = 0; i < fileSize; i += BUF_SIZE) {
        memset(buf, 0, BUF_SIZE);
        fread(buf, 1, BUF_SIZE, fPtr);

        writeContent(buf, strlen(buf), "Content Write");
    }

    fclose(fPtr);

    /* Wait for ACK from Server */
    memset(buf, 0, BUF_SIZE);
    if (read(descriptor, buf, BUFSIZ) == -1) {
        syserr("buffer read");
    }

    if (strcmp(buf, "ACK") == 0) {
        fprintf(stderr, "Got message...\n");
        exit(EXIT_SUCCESS);
    }
}

void a_read_cb(struct bufferevent *bev, void *arg) {
    char buf[BUF_SIZE + 1];

    while (evbuffer_get_length(bufferevent_get_input(bev))) {
        int r = bufferevent_read(bev, buf, BUF_SIZE);
        if (r == -1)
            syserr("bufferevent_read");
        buf[r] = 0;
        printf("\n--> %s\n", buf);
    }
}

void an_event_cb(struct bufferevent *bev, short what, void *arg) {
    if (what & BEV_EVENT_CONNECTED) {
        fprintf(stderr, "Connection made.\n");
        return;
    }
    if (what & BEV_EVENT_EOF)
        fprintf(stderr, "EOF encountered.\n");
    else if (what & BEV_EVENT_ERROR)
        fprintf(stderr, "Unrecoverable error.\n");
    else if (what & BEV_EVENT_TIMEOUT)
        fprintf(stderr, "A timeout occured.\n");
    if (event_base_loopbreak(base) == -1)
        syserr("event_base_loopbreak");
}

int main(int argc, char *argv[]) {
    /* Kontrola dokumentów ... */
    if (argc != 4)
        fatal("Usage: %s hostname port [file]\n", argv[0]);

    fileName = argv[3];

    // Jeśli chcemy, żeby wszystko działało nieco wolniej, ale za to
    // działało dla wejścia z pliku, to należy odkomentować linijki
    // poniżej, a zakomentować aktualne przypisanie do base
     struct event_config *cfg = event_config_new();
     event_config_avoid_method(cfg, "epoll");
     base = event_base_new_with_config(cfg);
     event_config_free(cfg);

//    base = event_base_new();
    if (!base)
        syserr("event_base_new");

    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!bev)
        syserr("bufferevent_socket_new");
    bufferevent_setcb(bev, a_read_cb, NULL, an_event_cb, (void *) bev);

    struct addrinfo addr_hints = {
            .ai_flags = 0,
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = 0,
            .ai_addrlen = 0,
            .ai_addr = NULL,
            .ai_canonname = NULL,
            .ai_next = NULL
    };
    struct addrinfo *addr;

    if (getaddrinfo(argv[1], argv[2], &addr_hints, &addr))
        syserr("getaddrinfo");

    if (bufferevent_socket_connect(bev, addr->ai_addr, addr->ai_addrlen) == -1)
        syserr("bufferevent_socket_connect");
    freeaddrinfo(addr);
    if (bufferevent_enable(bev, EV_READ | EV_WRITE) == -1)
        syserr("bufferevent_enable");

    struct event *stdin_event =
            event_new(base, 0, EV_READ | EV_PERSIST, stdin_cb, NULL);
    if (!stdin_event)
        syserr("event_new");
    if (event_add(stdin_event, NULL) == -1)
        syserr("event_add");

    printf("Entering dispatch loop.\n");
    if (event_base_dispatch(base) == -1)
        syserr("event_base_dispatch");
    printf("Dispatch loop finished.\n");

    bufferevent_free(bev);
    event_base_free(base);

    return 0;
}
