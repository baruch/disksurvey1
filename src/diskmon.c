#include "diskmon.h"
#include "common.h"

#include <scsi/sg.h>
#include <errno.h>
#include <memory.h>
#include <assert.h>

void disk_mon_wire(void* _disk)
{
    disk_entry_t* disk = _disk;
    wire_fd_state_t fd_state;
    int disk_running = 1;

    wire_log(WLOG_INFO, "Starting monitor of disk %s fd %d ptr %p", disk->dev_name, disk->fd, disk);
    wire_fd_mode_init(&fd_state, disk->fd);

    while (disk_running) {
        int ret;
        sg_io_hdr_t io;
        unsigned char sense[128];
        unsigned char cmd[6];

        wire_fd_wait_msec(1000);
        wire_log(WLOG_DEBUG, "ping %s", disk->dev_name);

        memset(cmd, 0, sizeof(cmd)); // TUR, all zeroes

        memset(&io, 0, sizeof(io));
        io.interface_id = 'S';
        io.dxfer_direction = SG_DXFER_NONE;
        io.cmd_len = sizeof(cmd);
        io.mx_sb_len = sizeof(sense);
        io.cmdp = cmd;
        io.sbp = sense;
        io.timeout = 30*1000; // 30 seconds
        io.flags = SG_FLAG_LUN_INHIBIT;

        ret = wio_write(disk->fd, &io, sizeof(io));
        if (ret < sizeof(io)) {
            wire_log(WLOG_ERR, "Error writing to device %s fd %d ret %d errno %d", disk->dev_name, disk->fd, ret, errno);
            break;
        }

        // Wait for reply
        while (1) {
            wire_fd_mode_read(&fd_state);
            wire_fd_wait(&fd_state);

            ret = wio_read(disk->fd, &io, sizeof(io));
            if (ret == -1) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;
                wire_log(WLOG_INFO, "Device %s fd %d error reading from device errno=%d", disk->dev_name, disk->fd, errno);
                disk_running = 0;
                break;
            }

            if (ret == 0) {
                wire_log(WLOG_INFO, "Device %s fd %d at EOF", disk->dev_name, disk->fd);
                disk_running = 0;
                break;
            }

            assert(ret == sizeof(io));
            // Got reply
            break;
        }

        // TODO: process reply
        if (!disk_running)
            break;

        if (io.status != 0) {
            wire_log(WLOG_INFO, "Device %s fd %d status %08x, breaking", disk->dev_name, disk->fd, io.status);
            break;
        }

        if (io.sb_len_wr != 0) {
            wire_log(WLOG_INFO, "Device %s fd %d sense returned, breaking", disk->dev_name, disk->fd);
            break;
        }
    }

    // close the disk fd and mark the entry as dead (fd == -1)
    wire_log(WLOG_INFO, "Finished monitor of disk %s fd %d", disk->dev_name, disk->fd);
    close(disk->fd);
    disk->fd = -1;
}
