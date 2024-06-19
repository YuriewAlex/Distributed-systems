#ifndef PA1_STARTER_CODE_PROCESS_WORK_H
#define PA1_STARTER_CODE_PROCESS_WORK_H

#include "ipc.h"
#include "banking.h"
#include <sys/types.h>

typedef struct {
    local_id  id;
    local_id  count_proc;
    int **read_desc;
    int **write_desc;
    BalanceState    b_state;
    BalanceHistory  b_history;
} pr_info;



void create_pipes (pr_info * pr);



void close_unused_pipes (pr_info * pr);

void wait_for_messages (pr_info * pr, MessageType type);
void send_start_msg (pr_info * pr, char *payload);
void send_done_msg (pr_info * pr, char *payload);

#endif //PA1_STARTER_CODE_PROCESS_WORK_H
