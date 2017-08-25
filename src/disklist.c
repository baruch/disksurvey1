#include "disklist.h"
#include "common.h"
#include "disk.h"
#include "diskmon.h"

#include <errno.h>
#include <string.h>

#define MAX_DISKS 256

struct disk_entry disks[MAX_DISKS];

void disklist_init(void)
{
    int i;

    for (i = 0; i < MAX_DISKS; i++)
        disks[i].fd = -1;
}

void disklist_add_device(const char* dev_name)
{
    int i;
    int empty_slot = -1;

    // Check for device existence
    for (i = 0; i < MAX_DISKS; i++) {
        if (disks[i].fd == -1) {
            empty_slot = i;
            continue;
        }

        if (strcmp(dev_name, disks[i].dev_name) == 0) {
            wire_log(WLOG_DEBUG, "Device %s already exists", dev_name);
            return;
        }
    }

    if (empty_slot == -1) {
        wire_log(WLOG_ERR, "No space to add device %s", dev_name);
        return;
    }

    wire_log(WLOG_INFO, "Adding device %s", dev_name);

    int fd = wio_open(dev_name, O_RDWR, 0);
    wire_log(WLOG_INFO,"Device opened %s", dev_name);
    if (fd < 0) {
        wire_log(WLOG_ERR, "Failed to open device %s errno %d", dev_name, errno);
        return;
    }

    strcpy(disks[empty_slot].dev_name, dev_name);
    disks[empty_slot].fd = fd;

    char name[128];
    snprintf(name, sizeof(name), "Monitor %s", dev_name);
    wire_run(name, disk_mon_wire, &disks[empty_slot]);
    wire_log(WLOG_INFO, "Started monitoring device %s", dev_name);
}
