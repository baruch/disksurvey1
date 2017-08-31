#include "diskmon.h"
#include "common.h"
#include "periodic_timers.h"

#include <scsicmd.h>

#include <scsi/sg.h>
#include <errno.h>
#include <memory.h>
#include <assert.h>
#include <stdbool.h>

static bool disk_mon_send_cmd(disk_entry_t* disk, sg_io_hdr_t* io, int dir, unsigned char* cmd, int cmd_len, unsigned char* sense, int sense_len, unsigned char* buf, int buf_sz)
{
    memset(io, 0, sizeof(*io));
    io->interface_id = 'S';
    io->dxfer_direction = dir;
    io->cmd_len = cmd_len;
    io->mx_sb_len = sense_len;
    io->dxfer_len = buf_sz;
    io->dxferp = buf;
    io->cmdp = cmd;
    io->sbp = sense;
    io->timeout = 30*1000;
    io->flags = SG_FLAG_LUN_INHIBIT;

    int ret = wio_write(disk->fd, io, sizeof(*io));
    if (ret < sizeof(io)) {
        wire_log(WLOG_ERR, "Error writing to device %s fd %d ret %d errno %d", disk->dev_name, disk->fd, ret, errno);
        return false;
    }
    return true;
}

static bool disk_mon_recv_reply(disk_entry_t* disk, sg_io_hdr_t* io, wire_fd_state_t* fd_state)
{
    // Wait for reply
    while (1) {
        wire_fd_mode_read(fd_state);
        wire_fd_wait(fd_state);

        int ret = wio_read(disk->fd, io, sizeof(*io));
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            wire_log(WLOG_INFO, "Device %s fd %d error reading from device errno=%d", disk->dev_name, disk->fd, errno);
            return false;
        }

        if (ret == 0) {
            wire_log(WLOG_INFO, "Device %s fd %d at EOF", disk->dev_name, disk->fd);
            return false;
        }

        assert(ret == sizeof(*io));
        // Got reply
        break;
    }

    return true;
}

int disk_mon_cmd(disk_entry_t* disk, wire_fd_state_t* fd_state, int dir, unsigned char* cmd, int cmd_len, unsigned char* sense, int sense_len, unsigned char* buf, int buf_sz)
{
    sg_io_hdr_t io;

    if (!disk_mon_send_cmd(disk, &io, dir, cmd, sizeof(cmd), sense, sizeof(sense), buf, buf_sz))
        return -1;

    if (!disk_mon_recv_reply(disk, &io, fd_state))
        return -1;

    // TODO: process reply, be more discernible regarding status values
    if (io.status != 0) {
        wire_log(WLOG_INFO, "Device %s fd %d status %08x, breaking", disk->dev_name, disk->fd, io.status);
        return -1;
    }

    if (io.sb_len_wr != 0) {
        // TODO: Parse the sense buffer
        wire_log(WLOG_INFO, "Device %s fd %d sense returned, breaking", disk->dev_name, disk->fd);
        return false;
    }

    return buf_sz - io.resid;
}

void disk_mon_wire(void* _disk)
{
    disk_entry_t* disk = _disk;
    wire_fd_state_t fd_state;
    unsigned char cmd[16];
    unsigned char sense[128];
    int buf_len;
    unsigned char buf[1024];
    int cmd_len;
    int device_type;
    scsi_vendor_t vendor;
    scsi_model_t model;
    scsi_fw_revision_t rev;
    scsi_serial_t serial;

    wire_log(WLOG_INFO, "Starting monitor of disk %s fd %d ptr %p", disk->dev_name, disk->fd, disk);
    wire_fd_mode_init(&fd_state, disk->fd);

    cmd_len = cdb_inquiry_simple(cmd, 96);
    buf_len = disk_mon_cmd(disk, &fd_state, SG_DXFER_FROM_DEV, cmd, cmd_len, sense, sizeof(sense), buf, 96);
    if (buf_len < 0) {
        wire_log(WLOG_ERR, "Error on inquiry command");
        goto Exit;
    }

    wire_log(WLOG_DEBUG, "inquiry len %d", buf_len);
    {
        int idx;
        int line_idx;

        for (idx = 0; idx < buf_len; idx++, line_idx++) {
            if (line_idx == 16) {
                printf("\n");
                line_idx = 0;
            }

            printf("%02x ", buf[idx]);
        }
    }
    if (!parse_inquiry(buf, buf_len, &device_type, vendor, model, rev, serial)) {
        wire_log(WLOG_INFO, "Inquiry data parsing failed");
        return;
    }
    wire_log(WLOG_INFO, "Disk %s type %d vendor %s model %s rev %s serial %s", disk->dev_name, device_type, vendor, model, rev, serial);

    while (1) {
        periodic_timer_wait(&timer_1sec);
        wire_log(WLOG_DEBUG, "ping %s", disk->dev_name);

        memset(cmd, 0, 6); // TUR, all zeroes
        if (disk_mon_cmd(disk, &fd_state, SG_DXFER_NONE, cmd, 6, sense, sizeof(sense), NULL, 0) < 0)
            break;
    }

    // close the disk fd and mark the entry as dead (fd == -1)
Exit:
    wire_log(WLOG_INFO, "Finished monitor of disk %s fd %d", disk->dev_name, disk->fd);
    close(disk->fd);
    disk->fd = -1;
}
