
#ifndef PA1_STARTER_CODE_LOGS_H
#define PA1_STARTER_CODE_LOGS_H
#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "process_work.h"

extern int pipelog1, eventlog1;

char *create_payload(MessageType type, pr_info* pr);

Message create_msg(MessageType type,  pr_info* pr);

void write_pipe(pr_info * pr,  int read, int write);

void write_closed_pipe(pr_info * pr,  int pipe);

void write_event(pr_info * pr, char *buffer);

void write_receive_done(pr_info * pr);

void write_recieve_started(pr_info * pr);

void write_smth(int fd, char  *buffer);

int start_logs();

int close_logs();

#endif //PA1_STARTER_CODE_LOGS_H
