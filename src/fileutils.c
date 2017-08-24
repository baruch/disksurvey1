#include "common.h"
#include "fileutils.h"

#include <assert.h>
#include <string.h>

void file_process_by_line_setup(file_process_state_t *state, int fd, char *buf, int buf_size)
{
    state->fd = fd;
    state->buf = buf;
    state->buf_size = buf_size;
    state->buf_len = 0;
    state->buf_offset = 0;
}

char* file_process_by_line(file_process_state_t *state)
{
    char *start_search = state->buf + state->buf_offset;

    while (1) {
        // Try to extract a line from the buffer
        if (state->buf_len > 0) {
            char *end = memchr(start_search, '\n', state->buf_len - state->buf_offset);
            if (end) {
                char *line = state->buf + state->buf_offset;
                state->buf_offset = end - state->buf + 1;
                if (state->buf_offset == state->buf_len) {
                    state->buf_offset = state->buf_len = 0;
                }
                *end = 0;
                return line;
            }

            start_search = state->buf + state->buf_len; // Next time search from the end of the current buffer
        }

        // Move the buffer to the start to have more space at the end
        if (state->buf_len > 0 && state->buf_offset > 0) {
            memmove(state->buf, state->buf + state->buf_offset, state->buf_len - state->buf_offset);
            start_search -= state->buf_offset;
            state->buf_offset = 0;
        }

        // If the buffer is full and there is was no eol found the line is too long, discard it
        if (state->buf_len == state->buf_size) {
            assert(state->buf_offset == 0);
            state->buf_len = 0;
            start_search = state->buf;
        }

        // If fd is marked invalid, bail out
        if (state->fd == -1)
            return NULL;

        // Refill the buffer
        int ret = wio_read(state->fd, state->buf + state->buf_len, state->buf_size - state->buf_len);
        if (ret <= 0) {
            state->fd = -1;

            if (state->buf_len == 0)
                return NULL; // No data in file and no line
            else {
                int null_offset;

                // Return the rest of the buffer as the last line (fd is now invalid)
                if (state->buf_offset + state->buf_len == state->buf_size)
                    null_offset = state->buf_size -1;
                else
                    null_offset = state->buf_offset + state->buf_len;
                state->buf[null_offset] = 0;
                char *line = state->buf + state->buf_offset;
                state->buf_len = state->buf_offset = 0;
                return line;
            }
        } else {
            // New data
            state->buf_len += ret;
        }
    }
}
