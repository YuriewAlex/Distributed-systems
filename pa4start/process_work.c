#define _GNU_SOURCE

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#include "process_work.h"
#include "logs.h"
#include "nonblock.h"




bool wait_for_message ( pr_info * pr) {
    Message msg;
    int from = receive_any(pr, &msg);
    //printf("Proc %i received msg of type %i from proc %i in time %i \n", pr->id,  msg.s_header.s_type, from, get_lamport_time());

     switch (msg.s_header.s_type) {
         case CS_REQUEST:
           push_to(&(pr->l_q), from, msg.s_header.s_local_time);

            Message reply = create_msg(CS_REPLY, pr);
           // printf("Proc %i CREATED REPLY %i to proc %i \n", pr->id, reply.s_header.s_type, from);
            if (send(pr, from, &reply) < 0){
                printf("Proc %i FAIL SENT REPLY to proc %i \n", pr->id, from);
                return false;
            } 
            //printf("Proc %i sent REPLY to proc %i \n", pr->id, from);
            break;

       
       

        case CS_REPLY:
            pr->info.rec_reply++;
            break;

        case CS_RELEASE:
            pop_from(&(pr->l_q), from);
            break;

        case DONE:
            pr->info.rec_done++;
            break;



        default:
            return false;
    }

    return true;

    
}



void create_pipes (pr_info * pr){
    int count_proc = pr->count_proc;

    //pr->read_desc = (int **)malloc(count_proc * sizeof(int *));
    //pr->write_desc = (int **)malloc(count_proc * sizeof(int *));
    for (int i = 0; i < count_proc; i++) {
        //pr->read_desc[i] = (int*)malloc(count_proc * sizeof(int));
        //pr->write_desc[i] = (int*)malloc(count_proc * sizeof(int));
    }

    for (local_id i = 0; i < count_proc; i++) {
        for (local_id j = 0; j < count_proc; j++) {
            if (i!=j) {
                int f_desc[2];
                if (pipe2(f_desc, O_NONBLOCK) == -1){
                    perror("pipe error");
                    close_logs();
                    exit(EXIT_FAILURE);
                }
                write_pipe(pr, f_desc[0], f_desc[1]);
                pr->read_desc[j][i] = f_desc[0];
                pr->write_desc[i][j] = f_desc[1];
            }
        }
    }


}


void close_unused_pipes ( pr_info * pr ) {
    for (local_id i = 0; i < pr->count_proc; i++) {
        for (local_id j = 0; j < pr->count_proc; j++) {
            if (i != j && i != pr->id) {
                    if (close(pr->read_desc[i][j]) != 0) {
                        perror("close_unused_pipes error");
                        close_logs();
                        exit(EXIT_FAILURE);
                    }
                    write_closed_pipe(pr, pr->read_desc[i][j]);


                    if (close(pr->write_desc[i][j]) != 0) {
                        perror("close_unused_pipes error");
                        close_logs();
                        exit(EXIT_FAILURE);
                    }
                    write_closed_pipe(pr, pr->write_desc[i][j]);


            }
        }
    }
}



local_id min_elem(local_queue * queue) {
    assert(queue->count > 0);

    local_id min_i;
    timestamp_t min_t = 16960;
    local_id min_id = MAX_PROCESS_ID;
    for (local_id i = 0; i < queue->count; i++) {
        if (queue->content[i].time <= min_t){
            if (queue->content[i].time == min_t){
                if (queue->content[i].id < min_id){
                    min_id = queue->content[i].id;
                    min_i = i;

                }
            }
            else {        
                min_t = queue->content[i].time;
                min_id = queue->content[i].id;
                min_i = i;
            }
        }
      
    }

    return min_i;
}



void push_to(local_queue * queue, local_id id, timestamp_t t) {
    queue->content[queue->count].id = id;
    queue->content[queue->count].time = t;
    queue->count++;
}

void pop_from(local_queue * queue, local_id id) {
    local_id min = min_elem(queue);
    assert(queue->content[min].id == id);
    queue->count--;
    queue->content[min] = queue->content[queue->count];
}

int request_cs(const void * self) {
     pr_info * pr = ( pr_info *) self;
    Message msg = create_msg(CS_REQUEST, pr);
    //printf("Proc %i started request \n", pr->id);
    

    int status = send_multicast(pr, &msg);
    if (status < 0) {
        return status;

    }
    //printf("Proc %i sended multicast \n", pr->id);

    push_to(&(pr->l_q), pr->id, get_lamport_time());
    pr->info.rec_reply = 0;

    while (pr->info.rec_reply < pr->count_proc-2 || pr->l_q.content[min_elem(&(pr->l_q))].id != pr->id) {
        if (!wait_for_message(pr)) {
            return -1;
        }
    }

    return 0;
}

int release_cs(const void * self) {
    pr_info * pr = (pr_info *) self;

    if (pr->l_q.count == 0) {
        return -1;
    }

    if (pr->l_q.content[min_elem(&(pr->l_q))].id != pr->id) {
        return -2;
    }

    pop_from(&(pr->l_q), pr->id);

    Message msg = create_msg(CS_RELEASE, pr);
    return send_multicast(pr, &msg);
}



