#include "common.h"
#include "diskscan.h"
#include "disklist.h"
#include "fileutils.h"

#include <sys/inotify.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define DEV_DIR "/dev/" // Use backslash at the end since some uses require it and others dont care

static char scratch_buf[4096];

static int sg_majors_read(uint8_t* sg_majors, int max_sg_majors)
{
    int fd;
    int num_sg_majors = 0;
    char buf[2048];
    char *line;

    fd = wio_open("/proc/devices", O_RDONLY, 0);
    if (fd < 0) {
        wire_log(WLOG_ERR, "Failed to open /proc/devices");
        return 0;
    }

    file_process_state_t state;
    file_process_by_line_setup(&state, fd, buf, sizeof(buf));
    int in_char_devs = 0;
    while ((line = file_process_by_line(&state)) != NULL) {
        if (strcmp(line, "Character devices:"))
            in_char_devs = 1;

        if (!in_char_devs)
            continue;

        if (line[0] == 0)
            break; // End of the character devices list

        char* dev_num = strtok(line, " \t\r");
        char* dev_name = strtok(NULL, " \t\r");
        if (strcmp(dev_name, "sg") == 0)
            sg_majors[num_sg_majors++] = atoi(dev_num);
    }

    wio_close(fd);

    return num_sg_majors;
}

static void diskscan_do_scan_device(const char* dev, uint8_t* sg_majors, int num_sg_majors)
{
    char dir_name[64];
    struct stat statbuf;
    int ret;
    char *str;

    str = stpcpy(dir_name, DEV_DIR);
    str = stpncpy(str, dev, sizeof(dir_name)-strlen(DEV_DIR));
    if (*str != 0) {
        wire_log(WLOG_ERR, "Failed to create device filename, it doesn't fit in the allotted space %s", dev);
        return;
    }

    ret = wio_stat(dir_name, &statbuf);
    if (ret < 0) {
        wire_log(WLOG_ERR, "Failed to state device %s errno %d", dir_name, errno);
        return;
    }

    if (statbuf.st_mode & S_IFMT != S_IFCHR)
        return;

    int dev_major = major(statbuf.st_rdev);
    int i;

    for (i = 0; i < num_sg_majors; i++) {
        if (dev_major == sg_majors[i]) {
            disklist_add_device(dir_name);
            return;
        }
    }
}

static void diskscan_do_scan(void)
{
    DIR* dir;
    struct dirent* entry;
    uint8_t sg_majors[8];
    int num_sg_majors;

    wire_log(WLOG_INFO, "Scanning disks");

    num_sg_majors = sg_majors_read(sg_majors, sizeof(sg_majors)/sizeof(sg_majors[0]));
    wire_log(WLOG_INFO, "majors %d first %d", num_sg_majors, sg_majors[0]);

    dir = wio_opendir(DEV_DIR);
    if (dir == NULL) {
        wire_log(WLOG_ERR, "Error opening directory " DEV_DIR);
        return;
    }

    while ((entry = wio_readdir(dir)) != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
        // A minor optimization if supported, we only want character devices
        if (entry->d_type != DT_CHR)
            continue;
#endif

        diskscan_do_scan_device(entry->d_name, sg_majors, num_sg_majors);
    }

    wio_closedir(dir);
    wire_log(WLOG_INFO, "Scan finished");
}

void diskscan_wire(void *arg)
{
    int fd;
    int wd = 0;
    wire_fd_state_t fd_state;

    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        wire_log(WLOG_FATAL, "Failed to initialize inotify");
        exit(1);
    }

    wd = inotify_add_watch(fd, DEV_DIR, IN_CREATE|IN_DELETE|IN_MOVED_TO|IN_MOVED_FROM);
    if (wd == -1) {
        wire_log(WLOG_FATAL, "Failed to set watch on " DEV_DIR);
        exit(1);
    }

    wire_fd_mode_init(&fd_state, fd);
    wire_fd_mode_read(&fd_state);

    while (1) {
        int ret;

        // Do the scan, here to cover the first time before any change needs to happen
        diskscan_do_scan();

        // Wait for a new event signalling a possible new device
        wire_fd_wait(&fd_state);

        // Clear the event buffer, we don't really care about its details
        // We also can aggregate multiple notifications into a single scan
        ret = read(fd, scratch_buf, sizeof(scratch_buf));
        if (ret == -1 && errno != EAGAIN) {
            wire_log(WLOG_FATAL, "Failed to read from watch fd");
            exit(1);
        }

        // TODO: use the information on what device was added/removed to handle
        // only this action instead of a full rescan
    }
}

void diskscan_init(void)
{
    wire_run("diskscan", diskscan_wire, NULL);
}
