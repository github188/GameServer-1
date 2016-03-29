#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "base.h"

static int id = USER_ENTID_MIN;
static struct base bs;

void onMessage(void *arg)
{
    char buffer[1024] = {0};
    int ret = read((int)arg, buffer, 1023);
    if (ret > 0) {
        printf("%s\n", buffer);
        write((int)arg, buffer, ret);
    }
}

void onConnect(void *arg)
{
    struct sockaddr_in clt_addr;
    socklen_t clt_addrlen = sizeof(clt_addr);
    int cltfd = accept((int)arg, (struct sockaddr*)&clt_addr, &clt_addrlen);
    printf("accepted a connect\n");

    struct event cltEnt;
    event_init(&cltEnt, cltfd, EV_READ, id++,onMessage, (void*)cltfd);
    event_register(&cltEnt, &bs);
}


int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("%s <port>\n", argv[0]);
        return 0;
    }
    base_init(&bs);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("create fd error\n");
        return -1;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("bind error\n");
        return -1;
    }

    if (listen(listenfd, 5) < 0) {
        printf("listen error\n");
        return -1;
    }

    struct event listenEnt;
    event_init(&listenEnt, listenfd, EV_READ, id++, onConnect, (void*)listenfd);
    event_register(&listenEnt, &bs);

    base_start(&bs);
    base_free(&bs);
    return 0;
}
