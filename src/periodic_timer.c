#include "periodic_timer.h"
#include "wire_fd.h"
#include "wire_wait.h"

#include "common.h"

#include <sys/timerfd.h>

struct periodic_timer_list {
    struct list_head list;
    wire_t* wire;
};

static void periodic_timer_wire(void *arg)
{
    periodic_timer_t *t = arg;
    wire_fd_state_t fd_state;

    wire_fd_mode_init(&fd_state, t->fd);

    while (1) {
        uint64_t val;

        wire_fd_mode_read(&fd_state);
        wire_fd_wait(&fd_state);

        read(t->fd, &val, sizeof(val));

        struct list_head* head;
        while ( (head = list_head(&t->head)) != NULL ) {
            list_del(head);
            struct periodic_timer_list *w = list_entry(head, struct periodic_timer_list, list);
            wire_resume(w->wire);
        }
    }
}

void periodic_timer_init(periodic_timer_t* t, int msecs)
{
    struct itimerspec timer;
    timer.it_interval.tv_sec = msecs / 1000;
    timer.it_interval.tv_nsec = (msecs % 1000) * 1000*1000;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_nsec = 1; // Start immediately

    t->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
    timerfd_settime(t->fd, 0, &timer, NULL);

    char name[32];
    snprintf(name, sizeof(name), "timer %d ms", msecs);
    t->wire = wire_run(name, periodic_timer_wire, t);
}

void periodic_timer_wait(periodic_timer_t* t)
{
    struct periodic_timer_list w;

    // Tell the timer wire to wake us up
    w.wire = wire_get_current();
    list_head_init(&w.list);
    list_add_tail(&t->head, &w.list);

    // Sleep on the wait
	wire_suspend();
}
