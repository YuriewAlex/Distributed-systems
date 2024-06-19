#define _GNU_SOURCE

#include "nonblock.h"
#include "banking.h"

timestamp_t curr_lamport = 0;

timestamp_t get_lamport_time(){return curr_lamport;}

timestamp_t inc_lamport(){ return ++curr_lamport;}

int cntr = 1;

bool nb_read(int fd, void * buf, size_t size_to_read) {
    uint8_t * start = buf;
    if (size_to_read == 0) return true;
    errno = 0;
    while (true) {
        const ssize_t bytes_read = read(fd, start, size_to_read);
        if (bytes_read < 0) {
            if (start != buf) {
                if (errno == EPIPE|| errno == EWOULDBLOCK) {
                    sched_yield();
                    continue;
                }
            }
            break;
        }
        if (bytes_read == 0) {
            if (errno == 0) errno = EPIPE;
            break;
        }
        size_to_read -= bytes_read;
        start += bytes_read;
        if (size_to_read == 0) return true;
    }
    return false;
}

bool nb_write(int fd,  const void * buf, size_t size_to_read) {
    //printf("WRITE %i\n", cntr);
    cntr++;
    const uint8_t * start = buf;
    if (size_to_read == 0) return true;
    errno = 0;
    while (true) {
        
        const ssize_t wrote = write(fd, start, size_to_read);
        if (wrote < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                sched_yield();
                continue;
            }
            break;
        }
        size_to_read -= wrote;
        start += wrote;
        if (size_to_read == 0) return true;
    }
    printf("FAILED WRITE \n");
    return false;
}



int receive_block (pr_info * pr, local_id id, Message * msg) {
    size_t loop_cnt = 0;
    while (true) {
        loop_cnt++;
        const int status = receive(pr, id, msg);
        if (status != 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                sched_yield();
                continue;
            }
        }
        //if(pr->id != PARENT_ID) printf("Proc %i just received this %i on loop %zu while RECEIVE_BLOCK\n", pr->id, msg->s_header.s_type, loop_cnt);
        return status;
    }
}

int receive_block_any(pr_info * pr, Message * msg){
    for (local_id id = 0; id < pr->count_proc; ++id) {
            if (id != pr->id) {
                if (receive_block(pr, id, msg) == 0) return 0;
                if (errno != EPIPE && errno != EWOULDBLOCK && errno != EAGAIN) return -1;
            }
        }
    return -1;
}
