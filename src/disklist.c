#include "disklist.h"
#include "common.h"
#include "disk.h"
#include "diskmon.h"

#include <errno.h>
#include <string.h>

#define MAX_DISKS 256

struct disk_entry disks[MAX_DISKS];

void disklist_add_device(const char* dev_name)
{
    int i;
    int empty_slot = 0;

    // Check for device existence
    for (i = 0; i < MAX_DISKS; i++) {
        if (disks[i].fd == -1)
            continue;

        empty_slot = i;

        if (strcmp(dev_name, disks[i].dev_name) == 0) {
            wire_log(WLOG_DEBUG, "Device %s already exists", dev_name);
            return;
        }
    }

    wire_log(WLOG_INFO, "Adding device %s", dev_name);

    int fd = wio_open(dev_name, O_RDWR, 0);
    if (fd < 0) {
        wire_log(WLOG_ERR, "Failed to open device %s errno %d", dev_name, errno);
        return;
    }

    strcpy(disks[i].dev_name, dev_name);
    disks[i].fd = fd;

    char name[128];
    snprintf(name, sizeof(name), "Monitor %s", dev_name);
    wire_run(name, disk_mon_wire, &disks[i]);
}
