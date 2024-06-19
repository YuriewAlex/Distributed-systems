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
    Message msg = create_msg (type, NULL, pr);
    while( send( pr, dst, &msg ) != 0 ) {};
}




void child_work(pr_info * pr){
    timestamp_t stop_time;
    timestamp_t last_tr;
    send_smb (pr, STARTED, PARENT_ID);
    int finished = 0;
    while( finished < pr->count_proc - 2 ) { 
        /*printf("Started new wait cycle proc %i \n", pr->id);
        printf("Curr pr time %i \n", pr->b_state.s_time);
        printf("Curr lamport time %i \n", get_lamport_time());*/
        Message msg;
        receive_any(pr, &msg);


        switch(msg.s_header.s_type) {

            case ACK: {
                //printf("Curr lamport time for proc %i is %i \n", pr->id, get_lamport_time());
                //printf("Proc %i received ACK msg with amount %i  with time %i \n", pr->id, atoi(msg.s_payload), msg.s_header.s_local_time);
                //pr->b_state.s_time = msg.s_header.s_local_time;
				balance_t amount = atoi(msg.s_payload);
				balance_t old_pending = pr->b_state.s_balance_pending_in;
				pr->b_state.s_balance_pending_in -= amount;
				for (timestamp_t t = pr->b_history.s_history_len; t < msg.s_header.s_local_time-1; ++t) {
                    pr->b_history.s_history[t].s_time = t;
					pr->b_history.s_history[t].s_balance = pr->b_state.s_balance;
					pr->b_history.s_history[t].s_balance_pending_in = old_pending;
				}
                pr->b_history.s_history[msg.s_header.s_local_time-1].s_time = msg.s_header.s_local_time-1;
				pr->b_history.s_history[msg.s_header.s_local_time-1].s_balance = pr->b_state.s_balance;
				pr->b_history.s_history[msg.s_header.s_local_time-1].s_balance_pending_in = pr->b_state.s_balance_pending_in;
				pr->b_history.s_history_len = msg.s_header.s_local_time;
                
				break;
            }
            
            case TRANSFER: {
                pr->b_state.s_time = get_lamport_time();

                TransferOrder* t_order = (TransferOrder*) msg.s_payload;

                if( t_order->s_src == pr->id ) {
                    char *payload = malloc(sysconf(_SC_PAGESIZE));
                    balance_t old_balance = pr->b_state.s_balance;
					balance_t old_pending = pr->b_state.s_balance_pending_in;
					pr->b_state.s_balance -= t_order->s_amount;
					pr->b_state.s_balance_pending_in += t_order->s_amount;
                    sprintf(payload, log_transfer_out_fmt, get_lamport_time(),  t_order->s_src,  t_order->s_amount, t_order->s_dst);
                    write_event(pr, payload);

                    for (timestamp_t t = pr->b_history.s_history_len; t < pr->b_state.s_time; ++t) {
                            pr->b_history.s_history[t].s_time = t;
							pr->b_history.s_history[t].s_balance = old_balance;
							pr->b_history.s_history[t].s_balance_pending_in = old_pending;
						}
                        pr->b_history.s_history[pr->b_state.s_time].s_time = pr->b_state.s_time;
						pr->b_history.s_history[pr->b_state.s_time].s_balance = pr->b_state.s_balance;	
						pr->b_history.s_history[pr->b_state.s_time].s_balance_pending_in = pr->b_state.s_balance_pending_in;
						pr->b_history.s_history_len = pr->b_state.s_time + 1;
                        /*pr->b_history.s_history[pr->b_state.s_time+1].s_time = pr->b_state.s_time;
						pr->b_history.s_history[pr->b_state.s_time+1].s_balance = pr->b_state.s_balance;	
						pr->b_history.s_history[pr->b_state.s_time+1].s_balance_pending_in = pr->b_state.s_balance_pending_in;
                        pr->b_state.s_balance_pending_in = 0;*/

                    
                    
                    Message next_msg = create_msg (TRANSFER, t_order, pr); 
                    while (send ( pr, t_order->s_dst, &next_msg ) != 0);
                    

                }

                if( t_order->s_dst == pr->id ) {
                    balance_t old_balance = pr->b_state.s_balance;
                    //pr->b_state.s_time = inc_lamport();
                    
					char* payload = malloc(sysconf(_SC_PAGESIZE));
                   

                    pr->b_state.s_balance += t_order->s_amount;
                    for (timestamp_t t = pr->b_history.s_history_len; t < pr->b_state.s_time; ++t) {
						pr->b_history.s_history[t].s_time = t;
                        pr->b_history.s_history[t].s_balance = old_balance;	
						pr->b_history.s_history[t].s_balance_pending_in = pr->b_state.s_balance_pending_in;
					}
                    pr->b_history.s_history[pr->b_state.s_time].s_time = pr->b_state.s_time;
					pr->b_history.s_history[pr->b_state.s_time].s_balance = pr->b_state.s_balance;
					pr->b_history.s_history[pr->b_state.s_time].s_balance_pending_in = pr->b_state.s_balance_pending_in;
					pr->b_history.s_history_len = pr->b_state.s_time + 1;
                    //printf("Proc %i start creating ACK msg, local time %i \n", pr->id, pr->b_state.s_time);
                    Message msg_ack_back;
                    pr->b_state.s_time = inc_lamport();
                    
                    //printf("Local time for proc %i is %i \n", pr->id, pr->b_state.s_time);
					sprintf(msg_ack_back.s_payload, "%d", t_order->s_amount);
					msg_ack_back.s_header.s_magic = MESSAGE_MAGIC;
					msg_ack_back.s_header.s_payload_len = strlen(msg_ack_back.s_payload);
					msg_ack_back.s_header.s_type = ACK;
					msg_ack_back.s_header.s_local_time = pr->b_state.s_time;
                    sprintf(payload, log_transfer_in_fmt, get_lamport_time()-1,  t_order->s_dst,  t_order->s_amount, t_order->s_src);
                    write_event(pr, payload);
                    last_tr = get_lamport_time();
                    
                    //printf("Proc %i created ACK msg for proc %i with amount %s and local time %i \n", pr->id, t_order->s_src, msg_ack_back.s_payload, msg_ack_back.s_header.s_local_time);
                    while(send( pr, t_order->s_src, &msg_ack_back) != 0 ) {};
                    send_smb (pr,  ACK, PARENT_ID);
                    
                }
                break;
            }

    
            case STOP: {
                stop_time = get_lamport_time();
                char* payload = create_payload(DONE, pr);
                write_event(pr, payload);
                send_all ( pr, DONE ); 
                break; }
            
            case DONE:
                ++finished;
                break;

            default:
                break;
        }
    }

    /*if (last_tr < stop_time) stop_time = last_tr;
    pr->b_state = pr->b_history.s_history[pr->b_history.s_history_len-1];
    pr->b_state.s_time = stop_time+1;
    for( timestamp_t t = pr->b_history.s_history_len; t < pr->b_state.s_time; t++ ) {
            pr->b_history.s_history[t] = pr->b_history.s_history[t-1];
            pr->b_history.s_history[t].s_time = t;
        }
        pr->b_history.s_history[pr->b_state.s_time] = pr->b_state;
        pr->b_history.s_history_len = pr->b_state.s_time;*/
    
    Message msg_b = create_msg (BALANCE_HISTORY, &pr->b_history, pr);
    
   
    while( send( pr, PARENT_ID, &msg_b) != 0 ){
    };



}


void process_birth (pr_info * pr, balance_t* all_balance) {

    for (local_id i = 0; i < pr->count_proc; i++) {
        pr->b_state.s_time = 0;
        if (i != PARENT_ID && pr->id == PARENT_ID) {
            pid_t pid = fork();
            if (pid == 0) {
                pr->id = i;                           
                pr->b_state.s_balance = all_balance[i-1];
                pr->b_state.s_balance_pending_in = 0;
                pr->b_history.s_history[0] = pr->b_state;
                pr->b_history.s_history_len = 1;
                pr->b_history.s_id = pr->id;
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
    /*char *payload = malloc(sysconf(_SC_PAGESIZE));
    sprintf(payload, log_transfer_out_fmt, get_lamport_time(),  src,  amount, dst);
    write_event(pr, payload);*/
    Message msg_rcv;
    receive_block(parent_data, dst, &msg_rcv);
    //printf("While waiting ACK Proc %i received msg of type %i in time %i \n", pr->id, msg_rcv.s_header.s_type, get_lamport_time());
    assert(msg_rcv.s_header.s_type == ACK);
    /*payload = malloc(sysconf(_SC_PAGESIZE));
    sprintf(payload, log_transfer_in_fmt, get_lamport_time(),  dst,  amount, src);
    write_event(pr, payload);*/
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
            return 1;
        }
         //printf("Proc %i received msg of type %i in time %i \n", ipc->id, msg.s_header.s_type, get_lamport_time());

        assert(msg.s_header.s_type == STARTED);
    }
    

        write_recieve_started(&this_process);

        bank_robbery(&this_process, this_process.count_proc - 1);

        send_all (&this_process, STOP);

        for (local_id id = 1; id < this_process.count_proc; ++id) {
        Message msg;

        if (receive_block(&this_process, id, &msg) < 0) {
            return 1;
        }
        //printf("While waiting Done Proc %i received msg of type %i in time %i \n", this_process.id, msg.s_header.s_type, get_lamport_time());

        assert(msg.s_header.s_type == DONE);
    }

        write_receive_done ( &this_process );

        AllHistory history;
        history.s_history_len = 0;


        while( history.s_history_len < this_process.count_proc - 1 ) {
   
            Message msg;
            receive_any( &this_process, &msg );

            if( msg.s_header.s_type == BALANCE_HISTORY ) {
                //printf("Proc %i received msg of type %i in time %i \n", this_process.id, msg.s_header.s_type, this_process.b_state.s_time);
                BalanceHistory temp;
                memcpy(&temp, &(msg.s_payload), sizeof(msg.s_payload));
                history.s_history[temp.s_id - 1] = temp;
                history.s_history_len++;
    
            }
        }
        int max_history_len = 0;
        for (int i = 0; i < this_process.count_proc-1; ++i) {
			if (history.s_history[i].s_history_len > max_history_len)
				max_history_len = history.s_history[i].s_history_len;
		}
		for (int i = 0; i < this_process.count_proc-1; ++i) {
			balance_t balance = history.s_history[i].s_history[history.s_history[i].s_history_len-1].s_balance;
			balance_t pending = history.s_history[i].s_history[history.s_history[i].s_history_len-1].s_balance_pending_in;
			for (int j = history.s_history[i].s_history_len; j < max_history_len; ++j) {
				history.s_history[i].s_history[j].s_balance = balance;
				history.s_history[i].s_history[j].s_balance_pending_in = pending;
				history.s_history[i].s_history[j].s_time = j;
			}
			history.s_history[i].s_history_len = max_history_len;
		}

        //print_history( &history );
        printf("history \n");
        for (size_t i = 0; i < history.s_history_len; i++)
        {
            printf("Proc %i ", i+1);
            for (size_t j = 0; j < history.s_history[i].s_history_len; j++)
            {
                printf("TIme= %i ||bal= %i ||pend= %i   ", history.s_history[i].s_history[j].s_time, history.s_history[i].s_history[j].s_balance, history.s_history[i].s_history[j].s_balance_pending_in);
            }
            printf("\n");
            
        }
        
        
        while(wait(NULL) > 0){
        }
        close_logs();
    }

    exit (0);

}
