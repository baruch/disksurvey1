#ifndef DISKSURVEY_COMMON_H
#define DISKSURVEY_COMMON_H

#include "wire.h"
#include "wire_fd.h"
#include "wire_pool.h"
#include "wire_log.h"
#include "wire_io.h"

extern wire_pool_t wire_pool;

static inline wire_t* wire_run(const char *name, void (*entry_point)(void*), void* arg)
{
    return wire_pool_alloc(&wire_pool, name, entry_point, arg);
}

#endif
