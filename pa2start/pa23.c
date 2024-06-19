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
    Message msg = create_msg ( type, NULL, pr);
    send_multicast( pr, &msg );
}

void send_smb ( pr_info* pr, MessageType type, local_id dst ) {
    char* payload = create_payload(type, pr);
    if (type != ACK) write_event(pr, payload);
    Message msg = create_msg ( type, NULL, pr);
    while( send( pr, dst, &msg ) != 0 ) {};
}


void add_balance_state_to_history (BalanceHistory* history, BalanceState state) {
    if (state.s_time < history->s_history_len -1){
        fprintf(stderr, "Critial error: cannot update balance state in the past - state time %hd, max time %d.\n", state.s_time, history->s_history_len -1);
        return;
    }
    if (state.s_time >= history->s_history_len -1){
        for( timestamp_t t = history->s_history_len; t < state.s_time; t++ ) {
            if (t == 0){
                fprintf(stderr, "Critial error: balance history does not have initial state");
            }
            history->s_history[t] = history->s_history[t-1];
            history->s_history[t].s_time = t;
        }
        history->s_history[state.s_time] = state;
        history->s_history_len = state.s_time + 1;
        
    }
}



void child_work(pr_info * pr){
    send_smb (pr, STARTED, PARENT_ID);
    int finished = 0;
    timestamp_t end_time = 0;
    while( finished < pr->count_proc - 2 ) { 
        Message msg;
        receive_any(pr, &msg);

       

        switch(msg.s_header.s_type) {
            
            case TRANSFER: {
                pr->b_state.s_time = get_physical_time();

                TransferOrder* t_order = (TransferOrder*) msg.s_payload;

                if( t_order->s_src == pr->id ) {
                    pr->b_state.s_balance -= t_order->s_amount;
                    add_balance_state_to_history(&(pr->b_history), pr->b_state);
                    while (send ( pr, t_order->s_dst, &msg ) != 0);
                }

                if( t_order->s_dst == pr->id ) {
                    pr->b_state.s_balance += t_order->s_amount;
                    add_balance_state_to_history(&(pr->b_history), pr->b_state);
                    send_smb (pr,  ACK, PARENT_ID);
                }

                break;
            }

    
            case STOP: {
                char* payload = create_payload(DONE, pr);
                write_event(pr, payload);
                send_all ( pr, DONE ); 
                end_time = get_physical_time();
                break; }
            
            case DONE:
                ++finished;
                break;

            default:
                break;
        }
    }

    pr->b_state = pr->b_history.s_history[pr->b_history.s_history_len-1];
    pr->b_state.s_time = end_time;
    add_balance_state_to_history(&(pr->b_history), pr->b_state);
    
   
   
    Message msg_b = create_msg (BALANCE_HISTORY, &pr->b_history, pr);
    
   
    while( send( pr, PARENT_ID, &msg_b) != 0 ){
    };



}


void process_birth (pr_info * pr, balance_t* all_balance) {

    for (local_id i = 0; i < pr->count_proc; i++) {
        if (i != PARENT_ID && pr->id == PARENT_ID) {
            pid_t pid = fork();
            if (pid == 0) {
         
                
                pr->id = i;
                
                pr->b_state.s_time = 0;
             
                pr->b_state.s_balance = all_balance[i-1];
              
                pr->b_state.s_balance_pending_in = 0;
         
                pr->b_history.s_history[0] = pr->b_state;
        
                pr->b_history.s_history_len = 1;
              
                pr->b_history.s_id = pr->id;
           
                close_unused_pipes(pr);
          
                child_work(pr);
                close_logs();   
                exit(0);
            }
        }
    }

}


void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
    pr_info* pr = parent_data;
    if( pr->id != PARENT_ID ) return; 
    TransferOrder t_order;
    t_order.s_src = src;
    t_order.s_dst = dst;
    t_order.s_amount = amount;
    Message msg_snd = create_msg ( TRANSFER, &t_order, pr);
    while( send( parent_data, src, &msg_snd ) != 0 ){};
    char *payload = malloc(sysconf(_SC_PAGESIZE));
    sprintf(payload, log_transfer_out_fmt, get_physical_time(),  src,  amount, dst);
    write_event(pr, payload);
    Message msg_rcv;
    while ( msg_rcv.s_header.s_type != ACK ) {
        receive_block( parent_data, dst, &msg_rcv );
    }
    payload = malloc(sysconf(_SC_PAGESIZE));
    sprintf(payload, log_transfer_in_fmt, get_physical_time(),  dst,  amount, src);
    write_event(pr, payload);
}

int main(int argc, char * argv[])
{   
    if (argc < 3) {
        printf( "[Too few arguments: expecting 3, actual %d]\n", argc);
        return 1;
    }
    if (strcmp(argv[1], "-p")) {
        printf("[Illegal option %s, expecting -p]\n", argv[1]);
        return 1;
    }
    local_id work_num = atoi(argv[2]);
    if (work_num <= 1 || work_num > MAX_PROCESS_ID) {
        printf("[Illegal number of child_processes %d, expecting 1 to %d]\n", work_num, MAX_PROCESS_ID);
        return 1;
    }
    balance_t all_balance[work_num];
    for (size_t i = 0; i < work_num; i++)
    {
        balance_t b = atoi(argv[i+3]);
        all_balance[i] = b;
    }
    
    
    if(start_logs() != 0) exit(EXIT_FAILURE);
    work_num++;
    pr_info this_process;
    this_process.id = PARENT_ID;
    this_process.count_proc = work_num;

    create_pipes(&this_process);

    process_birth(&this_process, all_balance);


    if (this_process.id == PARENT_ID) {
        close_unused_pipes(&this_process);
        for (local_id id = 1; id < this_process.count_proc; ++id) {
        Message msg;

        if (receive_block(&this_process, id, &msg) < 0) {
            return 0;
        }

        if(msg.s_header.s_type != STARTED) exit(1);
    }

        write_recieve_started(&this_process);

        bank_robbery(&this_process, this_process.count_proc - 1);

        send_all (&this_process, STOP);

        /*for (local_id id = 1; id < this_process.count_proc; ++id) {
        Message msg_d;

        if (receive_block(&this_process, id, &msg_d) < 0) {
            return 0;
        }

        if(msg_d.s_header.s_type != DONE) exit(1);}*/
        wait_for_messages(&this_process, DONE);

        write_receive_done ( &this_process );

        AllHistory history;
        history.s_history_len = 0;


        while( history.s_history_len < this_process.count_proc - 1 ) {
   
            Message msg;
            receive_any( &this_process, &msg );

            if( msg.s_header.s_type == BALANCE_HISTORY ) {
                BalanceHistory temp;
                memcpy(&temp, &(msg.s_payload), sizeof(msg.s_payload));
                history.s_history[temp.s_id - 1] = temp;
                history.s_history_len++;
    
            }
        }
        print_history( &history );
        
        while(wait(NULL) > 0){
        }
        close_logs();
    }

    exit (0);

}
