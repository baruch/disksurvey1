#pragma once

#include "common.h"
#include "wire_wait.h"
#include "list.h"

typedef struct {
    int fd;
    wire_t *wire;
    struct list_head head;
} periodic_timer_t;

void periodic_timer_init(periodic_timer_t* t, int msecs);
void periodic_timer_wait(periodic_timer_t* t);
