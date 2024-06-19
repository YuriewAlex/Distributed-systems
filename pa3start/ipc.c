#define _GNU_SOURCE
#include "ipc.h"
#include <unistd.h>
#include <stdio.h>
#include "process_work.h"
#include "nonblock.h"




int send(void * self, local_id dst, const Message * msg)
{
    pr_info* pr = self;
    if (!nb_write(pr->write_desc[pr->id][dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len)) return -1;
    return 0;
}

int send_multicast(void * self, const Message * msg)
{
    pr_info * pr = self;
    for (local_id id = 0; id < pr->count_proc; ++id) {
        if (id != pr->id) {
            if (send(pr, id, msg) != 0) return -1;
        }
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg)
{
    pr_info * pr = self;
    //printf("Proc %i started receiving msg from %i at time %i \n", pr->id, from, pr->b_state.s_time);
    if (!nb_read(pr->read_desc[pr->id][from], &(msg->s_header), sizeof(MessageHeader))) return -1;
    while (!nb_read(pr->read_desc[pr->id][from], msg->s_payload, msg->s_header.s_payload_len)) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            sched_yield();
            continue;
        }
        return -2;
    }
    //if (pr->id != PARENT_ID && msg->s_header.s_type == ACK) return 0;

    if (curr_lamport < msg->s_header.s_local_time) {
        curr_lamport = msg->s_header.s_local_time;
    }

    pr->b_state.s_time = inc_lamport();
    
    //printf("Proc %i DONE receiving msg from %i at time %i \n", pr->id, from, pr->b_state.s_time);
    return 0;
}

int receive_any(void * self, Message * msg)
{
    pr_info * pr = self;
     while (true) {
        for (local_id id = 0; id < pr->count_proc; ++id) {
            if (id != pr->id) {
                if (receive(pr, id, msg) == 0) return 0;
                if (errno != EPIPE && errno != EWOULDBLOCK && errno != EAGAIN) return -1;
            }
        }
        sched_yield();
    }
}


