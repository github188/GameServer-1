#include "base.h"
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

struct base base;

void onTimeOut(void *arg)
{
    printf("time out!%d\n", (int)arg);
    if ((int)arg == 8) {
        raise(SIGUSR1);
    }
    else if ((int)arg == 2) {
        struct event ent;
        event_init(&ent, time(NULL) + 2, EV_TIMER, USER_ENTID_MIN, onTimeOut, (void*)2);
        event_register(&ent, &base);
    }
}

void onSignal(void *arg)
{
    printf("SIGNAL on clicked\n");
}

void onPutin(void *arg)
{
    char buffer[64] = {0};
    int ret = read(0, buffer, 64);
    if (ret > 0) {
        printf("%s\n", buffer);
    }
}

int main()
{
    base_init(&base);
    
    struct event ent;
    event_init(&ent, time(NULL) + 2, EV_TIMER, USER_ENTID_MIN, onTimeOut, (void*)2);
    event_register(&ent, &base);

    struct event ent1;
    event_init(&ent, time(NULL) + 8, EV_TIMER, USER_ENTID_MIN + 1, onTimeOut, (void*)8);
    event_register(&ent, &base);

    struct event sigent;
    event_init(&sigent, SIGUSR1, EV_SIGNAL, USER_ENTID_MIN + 2, onSignal, NULL);
    event_register(&sigent, &base);

    struct event ioent;
    event_init(&ioent, 0, EV_READ, USER_ENTID_MIN + 3, onPutin, NULL);
    event_register(&ioent, &base);

    base_start(&base);

    base_free(&base);
    return 0;
}
