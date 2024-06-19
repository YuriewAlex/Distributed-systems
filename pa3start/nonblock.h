#ifndef PA1_STARTER_CODE_NONBLOCK_H
#define PA1_STARTER_CODE_NONBLOCK_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <wait.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sched.h>

#include "banking.h"
#include "pa2345.h"
#include "common.h"
#include "ipc.h"
#include "process_work.h"
#include "logs.h"

timestamp_t curr_lamport;

bool nb_read(int fd, void * buf, size_t remaining);

bool nb_write(int fd, const void * buf, size_t remaining);

int receive_block (pr_info * pr, local_id id, Message * msg);

int receive_block_any(pr_info * pr, Message * msg);

timestamp_t inc_lamport();

#endif
