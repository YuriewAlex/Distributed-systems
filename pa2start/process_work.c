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


void wait_for_messages ( pr_info * pr, MessageType type ) {
    int counter = 0;
    int total_proc;
    if (pr->id == PARENT_ID) total_proc = pr->count_proc - 1;
    else total_proc = pr->count_proc - 2;

    while ( counter < total_proc ) {
        Message msg;
        receive_any(pr, &msg);
        if (msg.s_header.s_type == type) counter++;
    } 
}



void create_pipes (pr_info * pr){
    int count_proc = pr->count_proc;

    pr->read_desc = (int **)malloc(count_proc * sizeof(int *));
    pr->write_desc = (int **)malloc(count_proc * sizeof(int *));
    for (int i = 0; i < count_proc; i++) {
        pr->read_desc[i] = (int*)malloc(count_proc * sizeof(int));
        pr->write_desc[i] = (int*)malloc(count_proc * sizeof(int));
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




