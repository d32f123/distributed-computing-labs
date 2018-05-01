#include "includes.h"
#include "pa1.h"
#include "communicator.h"
#include "logger.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>


local_id get_proc_num_from_args(char* argv[]);
void send_started_msg(Communicator* comm);
void send_done_msg(Communicator* comm);
void receive_all_msgs(Communicator* comm, MessageType msgType);

#define BUFFER_SIZE (512)
char buffer[BUFFER_SIZE];

int main(int argc, char* argv[])
{
    int* children;
    int fork_id;
    local_id my_id, i;
    local_id procsNum = 6;
    Communicator* comm;

    if (argc > 1)
        procsNum = get_proc_num_from_args(argv) + 1;
    // allocate space for children buffer
    children = malloc(sizeof(int) * (procsNum - 1));

    // init log
    log_init();
    // log start of parent process
    log_start(PARENT_ID);

    // generate all pipes between all processes
    comm = generate_communications(procsNum);

    
    // create children
    for (i = 0; i < procsNum - 1; ++i)
    {
        fork_id = fork();
        if (fork_id < 0)
            return fork_id;
        if (fork_id == 0) // if child
        {
            // free unused by chilfren buffer
            free(children);
            break;
        }
        children[i] = fork_id;
    }

    // set pipe communication and free unused fds
    if (fork_id != 0) // if the parent
    {
        my_id = PARENT_ID;
    } 
    else
    {
        my_id = i + 1;
    }
    set_communications_local(comm, my_id);
    log_pipes(comm);

    // send started msg if not parent
    if (my_id != PARENT_ID)
        send_started_msg(comm);

    // receive all started msgs from all other processes
    receive_all_msgs(comm, STARTED);

    // perform the actual work
    // spoilers: there is none whoops

    // tell all the processes that we are done
    if (my_id != PARENT_ID)
        send_done_msg(comm);
    receive_all_msgs(comm, DONE);

    // if parent, wait for children
    if (my_id == PARENT_ID)
    {
        for (i = 0; i < procsNum - 1; ++i)
            waitpid(children[i], NULL, 0);
    }
    // deinit logging
    log_destruct();

    // close all the pipes
    destroy_communicatons(comm);

    // if child, just go exit
    exit(0);
}

local_id get_proc_num_from_args(char* argv[])
{
    if (strcmp(argv[1], "-p") != 0)
        return -1;
    return atoi(argv[2]);
}

void send_started_msg(Communicator* comm)
{
    log_start(comm->curr_id);
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = STARTED;
    msg.s_header.s_local_time = time(NULL);

    // set string
    int length = snprintf(buffer, BUFFER_SIZE, log_started_fmt, comm->curr_id, getpid(), getppid());
    if (length <= 0)
    {
        perror("failed to apply snprintf");
        exit(-2);
    }
    msg.s_header.s_payload_len = length;
    memcpy(msg.s_payload, buffer, sizeof(char) * length);


    send_multicast(comm, &msg);
    
}

void send_done_msg(Communicator* comm)
{
    log_done(comm->curr_id);
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = 0;
    msg.s_header.s_type = DONE;
    msg.s_header.s_local_time = time(NULL);
    
    // set string
    int length = snprintf(buffer, BUFFER_SIZE, log_done_fmt, comm->curr_id);
    if (length <= 0)
    {
        perror("failed to apply snprintf");
        exit(-2);
    }
    msg.s_header.s_payload_len = length;
    memcpy(msg.s_payload, buffer, sizeof(char) * length);

    send_multicast(comm, &msg);
}


void receive_all_msgs(Communicator* comm, MessageType msgType)
{
    Message msg;
    int ret;
    int i;

    for (i = 1; i < comm->total_ids; ++i)
    {
        // skip yourself
        if (i == comm->curr_id)
            continue;
        
        ret = receive(comm, i, &msg);
        if (ret < 0)
        {
            perror("ERROR: Read from pipe failed badly");
        }
        // the thing about pipes is that if the writing process has already closed, 
        // reading from pipe even if there is something still in there
        // will result in EOL.
        // Therefore, we could assume that the process has sent us a DONE signal
        // and then closed
        if (msgType == DONE && ret == 0)
            continue;
        else if (ret == 0 && msgType != DONE)
        {
            perror("ERROR: RECEIVED WRONG MESSAGE FROM PIPE, PROCESS HAS ALREADY CLOSED");
        }
    }
    switch (msgType)
    {
        case STARTED:
            log_received_all_started(comm->curr_id);
            break;
        case DONE:
            log_received_all_done(comm->curr_id);
        default:
            break;
    }
}

