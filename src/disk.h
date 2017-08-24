#ifndef DISKSURVEY_DISK_H
#define DISKSURVEY_DISK_H

typedef struct disk_entry {
    char dev_name[64];
    int fd;
} disk_entry_t;

#endif
