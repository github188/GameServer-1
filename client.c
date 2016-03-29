#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base.h"

static int id = USER_ENTID_MIN;

void onPutin(void *arg)
{
    char buffer[1024] = {0};
    int ret = read(0, buffer, 1024);
    if (ret > 0) {
        write((int)arg, buffer, ret);
    }
}

void onMessage(void *arg)
{
    char buffer[1024] = {0};
    int ret = read((int)arg, buffer, 1024);
    if (ret > 0)
        printf("msg: %s\n", buffer);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("%s <ip> <port>\n", argv[0]);
        return 0;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("connect failure\n");
        close(sockfd);
        return -1;
    }

    struct base bs;
    base_init(&bs);

    struct event inEnt;
    event_init(&inEnt, 0, EV_READ, id++, onPutin, (void*)sockfd);
    event_register(&inEnt, &bs);

    struct event msgEnt;
    event_init(&msgEnt, sockfd, EV_READ, id++, onMessage, (void*)sockfd);
    event_register(&msgEnt, &bs);

    base_start(&bs);
    base_free(&bs);
    return 0;
}
