//
// Created by gor027 on 08.04.2020.
//

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zconf.h>
#include "err.h"

#define BUF_SIZE 1024
#define SIZE 65

void sending(int sock, char *buffer, size_t buf_size, const char *error) {
    if (write(sock, buffer, buf_size) < 0) {
        syserr(error);
    }
}

int main(int argc, char *argv[]) {
    struct addrinfo addr_hints, *addr_result;

    if (argc != 4) {
        fatal("Usage: %s host port file", argv[0]);
    }

    char *fileName = argv[3];

    FILE *ftPtr = fopen(fileName, "r");

    if (!ftPtr)
        syserr("Could not open the file!");

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0) {
        syserr("Socket");
    }

    /* Server Address */

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_flags = 0;
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;

    /*
     * argv[1] is the IP address
     * argv[2] is the port
     */
    int rc = getaddrinfo(argv[1], argv[2], &addr_hints, &addr_result);

    if (rc != 0) {
        fprintf(stderr, "rc=%d\n", rc);
        syserr("getaddrinfo: %s", gai_strerror(rc));
    }

    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) != 0)
        syserr("connect");

    freeaddrinfo(addr_result);

    /* Assumed that file name is not longer than BUF_SIZE */
    /* First, send the size of the file name */
    int fileNameSize = strlen(fileName);

    /* Writing to buffer, size is max 8 byte */
    char sizeInStr[SIZE];
    snprintf(sizeInStr, SIZE, "%064d", fileNameSize);

    sending(sock, sizeInStr, SIZE, "Size of the file...");

    /* Sending the name of the file */
    sending(sock, fileName, fileNameSize, "Sending file name...");

    /* Getting file size */
    fseek(ftPtr, 0L, SEEK_END);
    int fileSize = ftell(ftPtr);
    /* SEEK back */
    fseek(ftPtr, 0L, SEEK_SET);

    char fileSizeInStr[SIZE];
    snprintf(fileSizeInStr, SIZE, "%064d", fileSize);

    sending(sock, fileSizeInStr, SIZE, "File Size... ");

    /* Sending file ... */

    char buffer[BUF_SIZE];
    for (uint64_t i = 0; i < fileSize; i += BUF_SIZE) {
        memset(buffer, 0, BUF_SIZE);
        /* Reads a BUF_SIZE bits from file */
//        fgets(buffer, BUF_SIZE, ftPtr);
        fread(buffer, 1, BUF_SIZE, ftPtr);
        /* Size of buffer may be less than buffer */
        sending(sock, buffer, strlen(buffer), "Sending the file...");
    }

    if (close(sock) < 0)
        syserr("Close the socket ...");

    /* Closing the opened file */
    fclose(ftPtr);

    return 0;
}