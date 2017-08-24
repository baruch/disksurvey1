#include "diskmon.h"
#include "common.h"

void disk_mon_wire(void* _disk)
{
    disk_entry_t* disk = _disk;

    wire_log(WLOG_INFO, "Starting monitor of disk %s fd %d", disk->dev_name, disk->fd);

    // close the disk fd and mark the entry as dead (fd == -1)
    close(disk->fd);
    disk->fd = -1;
    wire_log(WLOG_INFO, "Finished monitor of disk %s fd %d", disk->dev_name, disk->fd);
}
