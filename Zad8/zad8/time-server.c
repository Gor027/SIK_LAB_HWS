//
// Created by gor027 on 09.05.2020.
//
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "time.h"

#include "err.h"

#define BSIZE 1024

void sendTimeToClient(int sock, struct sockaddr_in client_address) {
    char buffer[BSIZE];
    size_t length;
    time_t time_buffer;
    int sflags = 0;
    ssize_t snd_len;
    socklen_t snda_len = (socklen_t) sizeof(client_address);

    time(&time_buffer);
    strncpy(buffer, ctime(&time_buffer), BSIZE - 1);
    length = strnlen(buffer, BSIZE);
    snd_len = sendto(sock, buffer, length, sflags, (struct sockaddr *) &client_address, snda_len);

    if (snd_len != length) {
        syserr("error on sending datagram to client socket");
    }
}

int main(int argc, char *argv[]) {
    /* argumenty wywołania programu */
    char *multicast_dotted_address;
    in_port_t local_port;

    /* zmienne i struktury opisujące gniazda */
    int sock;
    struct sockaddr_in local_address;
    struct ip_mreq ip_mreq;

    /* zmienne obsługujące komunikację */
    char buffer[BSIZE];
    ssize_t rcv_len;

    if (argc != 3)
        fatal("Usage: %s multicast_dotted_address local_port\n", argv[0]);

    multicast_dotted_address = argv[1];
    local_port = (in_port_t) atoi(argv[2]);

    /* otwarcie gniazda */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    /* podłączenie do grupy rozsyłania (ang. multicast) */
    ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (inet_aton(multicast_dotted_address, &ip_mreq.imr_multiaddr) == 0) {
        fprintf(stderr, "ERROR: inet_aton - invalid multicast address\n");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0)
        syserr("setsockopt");

    /* ustawienie adresu i portu lokalnego */
    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    local_address.sin_port = htons(local_port);

    if (bind(sock, (struct sockaddr *) &local_address, sizeof local_address) < 0)
        syserr("bind");

    while (1) {
        struct sockaddr_in srvr_address;
        socklen_t rcva_len;
        int flags = 0;
        (void) memset(buffer, 0, sizeof(buffer));

        rcv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, flags, (struct sockaddr *) &srvr_address, &rcva_len);

        if (rcv_len < 0)
            syserr("read");

        if (strncmp(buffer, "GET TIME", rcv_len) != 0) {
            fprintf(stdout, "Received unknown command: %.*s\n", (int) rcv_len, buffer);
        } else {
            fprintf(stdout, "Request from: %s\n", inet_ntoa(srvr_address.sin_addr));
            sendTimeToClient(sock, srvr_address);
        }
    }

    exit(EXIT_SUCCESS);
}

