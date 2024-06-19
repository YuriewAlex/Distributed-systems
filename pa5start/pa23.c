#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <sys/wait.h>


#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "process_work.h"
#include "logs.h"
#include "banking.h"
#include "nonblock.h"




void send_all ( pr_info* pr, MessageType type ) {
    Message msg = create_msg ( type, pr);
    send_multicast( pr, &msg );
}

void send_smb ( pr_info* pr, MessageType type, local_id dst ) {
    Message msg = create_msg (type, pr);
    while( send( pr, dst, &msg ) != 0 ) {};
}




void child_work(pr_info * pr){

    
    send_all (pr, STARTED);
    




    for (local_id id = 1; id < pr->count_proc; ++id) {
        if (id == pr->id) {
            continue;
        }

        Message msg;
        receive_block(pr, id, &msg);

        assert(msg.s_header.s_type == STARTED);
    }
    //wait_for_messages(pr, STARTED);

    write_recieve_started(pr);



   
    int total = pr->id * 5;
    for (int i = 1; i <= total; i++)
    {
        //printf("Proc %i trying start iteration %i \n", pr->id, i);
        if (pr->info.is_mutex){
            
            if (request_cs(pr) < 0){
                printf("BAD REQUEST \n");
                return;
            }
        }
        //printf("PROC %i ENTER CRITICAL IN TIME %i\n", pr->id, pr->local_time);
        char* buf = malloc(sysconf(_SC_PAGESIZE));
        sprintf(buf, log_loop_operation_fmt, pr->id, i, total);
        print(buf);

        if (pr->info.is_mutex){
            if (release_cs(pr) < 0){
                printf("BAD RELEASE \n");
                return;
            }
        }

    }

    char* payload = create_payload(DONE, pr);
    write_event(pr, payload);
    send_all ( pr, DONE );

    while (pr->info.rec_done < pr->count_proc - 2) {
        if (!wait_for_message(pr)) {
            return;
        }
    }
    write_receive_done(pr);
    


}


void process_birth (pr_info * pr) {

    for (local_id i = 0; i < pr->count_proc; i++) {
        pr->local_time = 0;
        if (i != PARENT_ID && pr->id == PARENT_ID) {
            pid_t pid = fork();
            if (pid == 0) {
                pr->id = i;
                pr->info.rec_done = 0;
                pr->info.rec_reply = 0;
                pr->info.in_cstate = false;
                for (local_id j = 0; j < pr->count_proc; j++){
                    pr->delay[j] = 0;
                }                   
                close_unused_pipes(pr);
                char* payload = create_payload(STARTED, pr);
                write_event(pr, payload);
                child_work(pr);
                close_logs();   
                exit(0);
            }
        }
    }

}




int main(int argc, char * argv[])
{   
    if (argc < 3) {
        printf( "[Too few arguments: expecting 3, actual %d]\n", argc);
        return 1;
    }

    opterr = 0;

    int opt;
    local_id work_num;
    bool is_mutex = false;
    while ((opt = getopt(argc, argv, "p:")) > 0) {
        if (opt == 'p') {
            work_num = (local_id) (atoi(optarg));
            break;
        }
    }

 
    for (int i = 1; i < argc; ++i) {
        if (strcmp("--mutexl", argv[i]) == 0) {
            is_mutex = true;
        }
    }

    




    
    
    if(start_logs() != 0) exit(EXIT_FAILURE);
    work_num++;
    pr_info this_process;
    this_process.id = PARENT_ID;
    this_process.count_proc = work_num;
    this_process.info.is_mutex = is_mutex;
    this_process.info.rec_done = 0;

    

    create_pipes(&this_process);


    process_birth(&this_process);


    if (this_process.id == PARENT_ID) {
        
    
        close_unused_pipes(&this_process);

        for (local_id id = 1; id < this_process.count_proc; ++id) {
        Message msg;

        receive_block(&this_process, id, &msg);
  

        assert(msg.s_header.s_type == STARTED);
    }


        write_recieve_started(&this_process);


   
      
        
        while (this_process.info.rec_done < work_num - 1) {
           
            Message msg;

            receive_any(&this_process, &msg);
            if (msg.s_header.s_type == DONE) this_process.info.rec_done++;
        }


        write_receive_done ( &this_process );
      
        
        while(wait(NULL) > 0){
        }
        close_logs();
        
    }

    exit (0);

}
