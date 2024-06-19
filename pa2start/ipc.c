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
    if (!nb_read(pr->read_desc[pr->id][from], &(msg->s_header), sizeof(MessageHeader))) return -1;
    while (!nb_read(pr->read_desc[pr->id][from], msg->s_payload, msg->s_header.s_payload_len)) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            sched_yield();
            continue;
        }
        return -2;
    }
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
