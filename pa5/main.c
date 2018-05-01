#include "includes.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>

int donesLeft = 0;

#define BUFFER_SIZE (1024)
static char buffer[BUFFER_SIZE];

void do_parent_job(Communicator* comm);
void do_child_job(Communicator* comm, char withMutex);

local_id translate_args(char* withMutex, int argc, char* argv[]);

local_id create_children(pid_t* children, int num);

int main(int argc, char* argv[])
{
    pid_t* children;
    local_id procsNum;
    Communicator* comm;
    char withMutex;

    procsNum = translate_args(&withMutex, argc, argv);
    if (procsNum == -1)
    {
        perror("Usage: <prog> -p X [--mutexl]\n");
        return -1;
    }

    // allocate space for children buffer
    children = malloc(sizeof(int) * procsNum);

    // init log
    log_init();
    // log start of parent process
    log_start(PARENT_ID);

    // generate all pipes between all processes
    comm = generate_communications(procsNum + 1);

    // create children and
    // set pipe communication and free unused fds
    set_communications_local(comm, create_children(children, procsNum));
    if (comm->curr_id != PARENT_ID)
        free(children);
    log_pipes(comm);

    if (comm->curr_id == PARENT_ID)
        do_parent_job(comm);
    else
        do_child_job(comm, withMutex);

    // if parent, wait for children
    if (comm->curr_id == PARENT_ID)
    {
        for (int i = 0; i < procsNum; ++i)
            waitpid(children[i], NULL, 0);
    }
    // deinit logging
    log_destruct();

    // close all the pipes
    destroy_communicatons(comm);

    // if child, just go exit
    exit(0);
}

void do_parent_job(Communicator* comm)
{
    receive_all_msgs(comm, STARTED);

    donesLeft = comm->total_ids - 1;
    while (donesLeft > 0)
    {
        Message msg;

        while (receive_any(comm, &msg) <= 0);

        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, &msg);
    }
    log_received_all_done(comm->curr_id);
}


void do_child_job(Communicator* comm, char withMutex)
{
    printf("Child created. Physical time: %d\n", 0);

    donesLeft = comm->total_ids - 2;

    // send started msg
    increment_lamport_time();
    broadcast_started_msg(comm);

    // receive all started msgs from all other processes
    // no increment becuase it is already included in function call
    receive_all_msgs(comm, STARTED);

    // PAYLOAD:
    // main loop

    for (int i = 1; i <= comm->curr_id * 5; ++i)
    {
        if (withMutex)
            request_cs(comm);

        snprintf(buffer, BUFFER_SIZE, log_loop_operation_fmt, comm->curr_id, i, comm->curr_id * 5);
        print(buffer);

        if (withMutex)
            release_cs(comm);
    }

    // end phase: broadcast and receive all done messages
    broadcast_done_msg(comm);
    while (donesLeft > 0)
    {
        Message msg;

        while (receive_any(comm, &msg) <= 0);

        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, &msg);
    }
    log_received_all_done(comm->curr_id);

}

local_id translate_args(char* withMutex, int argc, char* argv[])
{
    local_id procsNum = -1;
    char mutexRet = 0;
    for (int i = 1; i < argc; ++i) 
    {
        if (!strcmp("-p", argv[i]))
        {
            procsNum = atoi(argv[++i]);
            continue;
        }
        if (!strcmp("--mutexl", argv[i]))
        {
            mutexRet = 1;
            continue;
        }
    }

    *withMutex = mutexRet;
    return procsNum;
}

local_id create_children(pid_t* children, int num)
{
    pid_t fork_id;
    int i;
    for (i = 0; i < num; ++i)
    {
        fork_id = fork();
        if (fork_id < 0)
            return fork_id;
        if (fork_id == 0) // if child
        {
            break;
        }
        children[i] = fork_id;
    }

    // set pipe communication and free unused fds
    if (fork_id != 0) // if the parent
    {
        return PARENT_ID;
    } 
    else
    {
        return i + 1;
    }
}

