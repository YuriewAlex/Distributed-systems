#ifndef PA1_STARTER_CODE_PROCESS_WORK_H
#define PA1_STARTER_CODE_PROCESS_WORK_H

#include "ipc.h"
#include "banking.h"
#include <sys/types.h>
#include <stdbool.h>

typedef struct 
{
     struct {
        timestamp_t time;
        local_id id;
    } content[MAX_PROCESS_ID+1];

    local_id count;
} local_queue;

typedef struct {
    local_id  id;
    local_id  count_proc;
    int read_desc[MAX_PROCESS_ID+1][MAX_PROCESS_ID+1];
    int write_desc[MAX_PROCESS_ID+1][MAX_PROCESS_ID+1];
    timestamp_t local_time;
    local_queue l_q;
    struct{
    bool is_mutex;
    int rec_done;
    int rec_reply;
    int rec_start;
    } info;
} pr_info;



void create_pipes (pr_info * pr);



void close_unused_pipes (pr_info * pr);

bool wait_for_message (pr_info * pr);
void send_start_msg (pr_info * pr, char *payload);
void send_done_msg (pr_info * pr, char *payload);
local_id min_elem(local_queue * queue);
void push_to(local_queue * queue, local_id id, timestamp_t t);
void pop_from(local_queue * queue, local_id id);


#endif //PA1_STARTER_CODE_PROCESS_WORK_H
