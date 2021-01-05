//
// Created by gor027 on 09.05.2020.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "poll.h"

#include "err.h"

#define BSIZE 1024
#define TTL_VALUE 4
#define WAIT_TIME 3000

int trialsCount = 0;

void sendMessage(int sock, struct sockaddr_in remote_address) {
    /* zmienne obsługujące komunikację */
    char buffer[BSIZE];
    sprintf(buffer, "GET TIME");
    size_t len = strnlen("GET TIME", BSIZE);
    ssize_t snd_len;
    int sflags = 0;

    snd_len = sendto(sock, buffer, strlen(buffer), sflags, (struct sockaddr *) &remote_address,
                     (socklen_t) sizeof(remote_address));

    if (snd_len != (ssize_t) len) {
        syserr("partial / failed write");
    }
}

void getTimeAndIP(int sock) {
    char buffer[BSIZE];
    int flags = 0;
    size_t len;
    ssize_t rcv_len;
    struct sockaddr_in srvr_address;

    memset(buffer, 0, sizeof(buffer));
    len = (size_t) sizeof(buffer) - 1;
    socklen_t rcva_len = (socklen_t) sizeof(srvr_address);
    rcv_len = recvfrom(sock, buffer, len, flags, (struct sockaddr *) &srvr_address, &rcva_len);

    if (rcv_len < 0) {
        syserr("read");
    }

    fprintf(stdout, "Response from: %s\n", inet_ntoa(srvr_address.sin_addr));
    fprintf(stdout, "Received time: %.*s\n", (int) rcv_len, buffer);
}

int main(int argc, char *argv[]) {
    /* argumenty wywołania programu */
    char *remote_dotted_address;
    in_port_t remote_port;

    /* zmienne i struktury opisujące gniazda */
    int sock, optval;
    struct sockaddr_in remote_address;

    /* parsowanie argumentów programu */
    if (argc != 3)
        fatal("Usage: %s remote_address remote_port\n", argv[0]);

    remote_dotted_address = argv[1];
    remote_port = (in_port_t) atoi(argv[2]);

    /* otwarcie gniazda */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    /* uaktywnienie rozgłaszania (ang. broadcast) */
    optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &optval, sizeof optval) < 0)
        syserr("setsockopt broadcast");

    /* ustawienie TTL dla datagramów rozsyłanych do grupy */
    optval = TTL_VALUE;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &optval, sizeof optval) < 0)
        syserr("setsockopt multicast ttl");

    /* ustawienie adresu i portu odbiorcy */
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(remote_port);
    if (inet_aton(remote_dotted_address, &remote_address.sin_addr) == 0) {
        fprintf(stderr, "ERROR: inet_aton - invalid multicast address\n");
        exit(EXIT_FAILURE);
    }

    struct pollfd pfds[1];
    pfds[0].fd = sock; /* Socket */
    pfds[0].events = POLLIN; /* Inform when ready to read */

    while (1) {
        if (trialsCount >= 3)
            break;

        sendMessage(sock, remote_address);
        ++trialsCount;
        fprintf(stdout, "Sending request [%d]\n", trialsCount);

        int num_events = poll(pfds, 1, WAIT_TIME);

        if (num_events == 0) {
            /* Timed out */
            continue;
        } else {
            int pollin_happend = pfds[0].revents & POLLIN;

            if (pollin_happend) {
                /* Descriptor is ready to read */
                getTimeAndIP(sock);
                close(sock);
                exit(EXIT_SUCCESS);
            } else {
                syserr("poll unexpected error");
            }
        }
    }

    fprintf(stdout, "Timeout. unable to receive response.\n");
    close(sock);
    exit(EXIT_SUCCESS);
}