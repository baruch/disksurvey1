#include "common.h"
#include "diskscan.h"

static wire_thread_t wire_thread;
wire_pool_t wire_pool;

void disksurvey_init(void *a)
{
    diskscan_init();
    wire_fd_wait_msec(1000);
}

int main()
{
    wire_thread_init(&wire_thread);
    wire_fd_init();
    wire_pool_init(&wire_pool, NULL, 256, 4096);
    wire_io_init(16);
    wire_log_init_stderr();

    wire_run("init", disksurvey_init, NULL);

    wire_thread_run();
    return 0;
}
