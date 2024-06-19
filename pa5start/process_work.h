#ifndef PA1_STARTER_CODE_PROCESS_WORK_H
#define PA1_STARTER_CODE_PROCESS_WORK_H

#include "ipc.h"
#include "banking.h"
#include <sys/types.h>
#include <stdbool.h>


typedef struct {
    local_id  id;
    local_id  count_proc;
    int read_desc[MAX_PROCESS_ID+1][MAX_PROCESS_ID+1];
    int write_desc[MAX_PROCESS_ID+1][MAX_PROCESS_ID+1];
    timestamp_t local_time;
    timestamp_t freeze_time;
    int delay[MAX_PROCESS_ID+1];
    struct{
    bool is_mutex;
    bool in_cstate;
    int rec_done;
    int rec_reply;
    } info;
} pr_info;



void create_pipes (pr_info * pr);



void close_unused_pipes (pr_info * pr);

bool wait_for_message (pr_info * pr);
void send_start_msg (pr_info * pr, char *payload);
void send_done_msg (pr_info * pr, char *payload);
int reply(pr_info * pr, local_id src);


#endif //PA1_STARTER_CODE_PROCESS_WORK_H
