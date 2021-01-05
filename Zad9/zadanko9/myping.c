//
// Created by gor027 on 16.05.2020.
//

#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>

#define MILLION 1000000
#define BSIZE 1000
#define ICMP_HEADER_LEN 8

unsigned short in_cksum(unsigned short *addr, int len);

void drop_to_nobody();

int isInterrupted = 0;
int icmpSentCount = 0;

void sigintInterrupt(int sig) {
    isInterrupted = 1;
}

/* timeval.tv_sec is of type long, so function returns bigger size long long. */
long long getCurrentTime() {
    struct timeval timeval;
    gettimeofday(&timeval, 0);
    return timeval.tv_sec * MILLION + timeval.tv_usec;
}

/* Sends request with current timestamp & icmp_seq */
void pingRequest(int sock, char *s_send_addr) {
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;
    struct sockaddr_in send_addr;

    struct icmp *icmp;

    char send_buffer[BSIZE];

    int err = 0;
    ssize_t data_len = 0;
    ssize_t icmp_len = 0;
    ssize_t len = 0;

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_RAW;
    addr_hints.ai_protocol = IPPROTO_ICMP;
    err = getaddrinfo(s_send_addr, 0, &addr_hints, &addr_result);
    if (err != 0) {
        syserr("getaddrinfo: %s\n", gai_strerror(err));
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = ((struct sockaddr_in *) (addr_result->ai_addr))->sin_addr.s_addr;
    send_addr.sin_port = htons(0);
    freeaddrinfo(addr_result);

    memset(send_buffer, 0, sizeof(send_buffer));
    icmp = (struct icmp *) send_buffer;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = htons(getpid());
    icmp->icmp_seq = htons(icmpSentCount++);

    data_len = snprintf(((char *) send_buffer + ICMP_HEADER_LEN), sizeof(send_buffer) - ICMP_HEADER_LEN, "%lld",
                        getCurrentTime());
    if (data_len < 1) {
        syserr("snprintf");
    }

    icmp_len = data_len + ICMP_HEADER_LEN;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum((unsigned short *) icmp, icmp_len);

    len = sendto(sock, (void *) icmp, icmp_len, 0, (struct sockaddr *) &send_addr, (socklen_t) sizeof(send_addr));
    if (icmp_len != (ssize_t) len) {
        syserr("partial / failed write");
    }
}

int pingReply(int sock) {
    struct sockaddr_in rcv_addr;
    socklen_t rcv_addr_len;

    struct ip *ip;
    struct icmp *icmp;

    char rcv_buffer[BSIZE];

    ssize_t ip_header_len = 0;
    ssize_t icmp_len = 0;
    ssize_t len;

    memset(rcv_buffer, 0, sizeof(rcv_buffer));
    rcv_addr_len = (socklen_t) sizeof(rcv_addr);
    len = recvfrom(sock, (void *) rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr *) &rcv_addr, &rcv_addr_len);

    if (len == -1) {
        syserr("failed read");
    }

    ip = (struct ip *) rcv_buffer;
    ip_header_len = ip->ip_hl << 2;

    icmp = (struct icmp *) (rcv_buffer + ip_header_len);
    icmp_len = len - ip_header_len;

    /* Bytes and reply address */
    fprintf(stdout, "%zd bytes from %s ", len, inet_ntoa(rcv_addr.sin_addr));

    /* TTL in the reply pack */
    fprintf(stdout, "icmp_seq=%d ttl=%d ", ntohs(icmp->icmp_seq), ip->ip_ttl);

    if (icmp_len < ICMP_HEADER_LEN) {
        fatal("icmp header len (%d) < ICMP_HEADER_LEN", icmp_len);
    }

    if (icmp->icmp_type != ICMP_ECHOREPLY) {
        printf("strange reply type (%d)\n", icmp->icmp_type);
        return 0;
    }

    if (ntohs(icmp->icmp_id) != getpid()) {
        fatal("reply with id %d different from my pid %d", ntohs(icmp->icmp_id), getpid());
    }

    ssize_t timePointer = ip_header_len + ICMP_HEADER_LEN;
    const char *pointerToTimeInBuffer = rcv_buffer + timePointer;
    long long requestPackTime = strtoll(pointerToTimeInBuffer, NULL, 10);
    long long currentTime = getCurrentTime();
    double time = (double) (currentTime - requestPackTime) / (double) 1000;

    /* Significance of .001 */
    fprintf(stdout, "time=%.3f ms\n", time);

    return 1;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fatal("Usage: %s host\n", argv[0]);
    }

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sock < 0)
        syserr("socket");

    /* Setting SIGINT signal service */
    sigset_t block_mask;
    struct sigaction action;

    sigemptyset(&block_mask);
    action.sa_handler = sigintInterrupt;
    action.sa_mask = block_mask;
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &action, 0) != 0) {
        syserr("sigaction");
    }

    drop_to_nobody();

    while (isInterrupted == 0) {
        pingRequest(sock, argv[1]);

        while (!pingReply(sock));

        /* Sleep may be added to not print so fast. */
//        sleep(1);
    }

    if (close(sock) < 0)
        syserr("close");

    return 0;
}
