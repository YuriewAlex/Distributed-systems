#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "process_work.h"
#include "nonblock.h"
int pipelog1, eventlog1;

int start_logs(){
    pipelog1 = open(pipes_log, O_CREAT | O_RDWR | O_APPEND, 0777);
    eventlog1 = open(events_log, O_CREAT | O_RDWR | O_APPEND, 0777);
    if (pipelog1 == -1 || eventlog1 == -1) {
        printf("Very bad\n");
        return -1;
    }
    return 0;
}

int close_logs(){
    close(pipelog1);
    close(eventlog1);
    return 0;
}

void write_smth(int fd, char  *buffer){
    write(fd, buffer, strlen(buffer));
}

void write_pipe(pr_info * pr,  int read, int write){
    char *buffer = malloc(sysconf(_SC_PAGESIZE));
    sprintf(buffer, "Process %d OPENED pipe (read fd %d, write fd %d)\n", pr->id, read, write);
    write_smth(pipelog1, buffer);

}

void write_closed_pipe(pr_info * pr,  int pipe){
    char *buffer = malloc(sysconf(_SC_PAGESIZE));
    sprintf(buffer, "Process %d CLOSED fd %d\n", pr->id, pipe);
    write_smth(pipelog1, buffer);
}

void write_event(pr_info * pr, char *buffer){
    write(STDOUT_FILENO, buffer, strlen(buffer));
    write_smth(eventlog1, buffer);
}

void write_receive_done(pr_info * pr){
    char *buffer = malloc(sysconf(_SC_PAGESIZE));
    sprintf(buffer, log_received_all_done_fmt, get_lamport_time(),  pr->id);
    write_event(pr, buffer);
}

void write_recieve_started(pr_info * pr){
    char *buffer = malloc(sysconf(_SC_PAGESIZE));
    sprintf(buffer, log_received_all_started_fmt,  get_lamport_time(), pr->id);
    write_event(pr, buffer);
}




char *create_payload(MessageType type, pr_info * pr)
{
    char *payload = malloc(sysconf(_SC_PAGESIZE));
    switch (type)
    {
        case STARTED:
            sprintf(payload, log_started_fmt, pr->local_time,  pr->id, getpid(), getppid(),0);
            break;
        case DONE:
            sprintf(payload, log_done_fmt, pr->local_time,  pr->id, 0);
            break;
        case STOP:
            payload = NULL;
            break;
        case ACK:
            payload = NULL;
            break;
        default:
            perror("Unknown msg type error");
            close_logs();
            exit(EXIT_FAILURE);
    }
    return payload;
}

Message create_msg(MessageType type, pr_info* pr){

    pr->local_time = inc_lamport();
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = 0;
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = get_lamport_time();
    
    return msg;





}
