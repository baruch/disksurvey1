#ifndef DISKSURVEY_FILEUTILS_H
#define DISKSURVEY_FILEUTILS_H

typedef struct file_process_state {
    int fd;
    int buf_len;
    int buf_offset;
    char *buf;
    int buf_size;
} file_process_state_t;

void file_process_by_line_setup(file_process_state_t *state, int fd, char *buf, int buf_size);
char* file_process_by_line(file_process_state_t *state);

#endif
