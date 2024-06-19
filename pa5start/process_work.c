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


     switch (msg.s_header.s_type) {
         case CS_REQUEST:
           if (pr->info.in_cstate){
                if ((msg.s_header.s_local_time < pr->freeze_time)  || (from < pr->id  && pr->freeze_time == msg.s_header.s_local_time)) {
                        if (!reply(pr, from)) {
                            return false;
                        }} 
                else {
             
                    pr->delay[from] = 1;}     
           }
            else {
                if (!reply(pr, from)) {
                    return false;}
           }

            break;

       
       

        case CS_REPLY:
            pr->info.rec_reply++;
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

int reply(pr_info* pr, local_id src){
    Message reply = create_msg(CS_REPLY, pr);
     if (send(pr, src, &reply) < 0){
                return 0;}
    return 1;
}









int request_cs(const void * self) {
     pr_info * pr = ( pr_info *) self;
    Message msg = create_msg(CS_REQUEST, pr);

    

    int status = send_multicast(pr, &msg);
    if (status < 0) {
        return status;

    }


    pr->freeze_time = get_lamport_time();
    pr->info.in_cstate = true;
    pr->info.rec_reply = 0;

    while (pr->info.rec_reply < pr->count_proc-2) {
        if (!wait_for_message(pr)) {
            return -1;
        }
    }

    return 0;
}

int release_cs(const void * self) {
    pr_info * pr = (pr_info *) self;

    if (!pr->info.in_cstate) {
        return -1;
    }

     for (local_id i = 1; i < pr->count_proc; i++) {
        if (pr->delay[i] == 1){
            reply(pr, i);
            pr->delay[i] = 0;
        }
     }
    

    pr->info.in_cstate = false;

   
    return 0;
}



