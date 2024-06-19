#define _GNU_SOURCE
#include "ipc.h"
#include <unistd.h>
#include <stdio.h>
#include "process_work.h"
#include "nonblock.h"




int send(void * self, local_id dst, const Message * msg)
{
    
    pr_info* pr = self;
    
    
    
    if (!nb_write(pr->write_desc[pr->id][dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len)){
         return -1;}

    return 0;
}

int send_multicast(void * self, const Message * msg)
{
    pr_info * pr = self;
    for (local_id id = 0; id < pr->count_proc; ++id) {
        if (id != pr->id) {
            if (send(pr, id, msg) != 0) return -1;
            //printf("Proc %i just sent msg of type %i to proc %i via multicast\n", pr->id, msg->s_header.s_type, id);
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

    if (curr_lamport < msg->s_header.s_local_time) {
        curr_lamport = msg->s_header.s_local_time;
    }

    pr->local_time = inc_lamport();
   
    return 0;
}

int receive_any(void * self, Message * msg)
{
    pr_info * pr = self;
     while (true) {
        for (local_id id = 0; id < pr->count_proc; ++id) {
            if (id != pr->id) {
                int recived = receive(pr, id, msg);
                if (recived == 0){
           
                    return id;
                }
                //printf("Proc %i just received this %i\n", pr->id, recived);
                if (errno != EPIPE && errno != EWOULDBLOCK && errno != EAGAIN) return -1;
            }
        }
        //break;
        sched_yield();
    }
}



/*int send(void * self, local_id dst, const Message * msg)
{
    pr_info * pr = self;
    if (msg == NULL)
        return -1;
    if (write(pr->write_desc[pr->id][dst],
              msg,
              sizeof(MessageHeader) +
              (msg->s_header.s_payload_len)) < 0)
        return -2;
    return 0;

}

int send_multicast(void * self, const Message * msg)
{
    pr_info * pr = self;
    if (msg == NULL)
        return -1;
    for (local_id i = 0; i < pr->count_proc; i++) {
        if (i != pr->id)
            if(send(pr, i, msg) != 0) return -2;
    }
    return 0;

 
}

int receive(void * self, local_id from, Message * msg)
{
    pr_info * pr = self;
    MessageHeader hdr;
    //if (msg == NULL)    return -1;
    if ((read(pr->read_desc[pr->id][from],
              &hdr,
              sizeof(MessageHeader))) == sizeof(MessageHeader))
    {
        msg->s_header = hdr;
        if (hdr.s_payload_len != 0){
        if (read(pr->read_desc[pr->id][from],
                 msg->s_payload,
                 hdr.s_payload_len) == hdr.s_payload_len){
                     if (curr_lamport < msg->s_header.s_local_time) {
        curr_lamport = msg->s_header.s_local_time;
    }

    pr->b_state.s_time = inc_lamport();
            return 0;}
        else
            return -2;
        }
         if (curr_lamport < msg->s_header.s_local_time) {
        curr_lamport = msg->s_header.s_local_time;
    }

    pr->b_state.s_time = inc_lamport();
        return 0;
    }
    return -3;

   
}

int receive_any(void * self, Message * msg)
{
    pr_info * pr = self;
    while(1){
        for (local_id from = 0; from < pr->count_proc; from++) {
            if ( from != pr->id ) {
                int recived = receive(pr, from, msg);
                printf("Proc %i just received this %i\n", pr->id, recived);
                if (recived == 0) return 0;
            }
        } 
    }
    
}*/

