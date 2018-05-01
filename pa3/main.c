#include "includes.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>

void do_parent_job(Communicator* comm);
void do_child_job(Communicator* comm, char* argv[]);

void do_transfer(Communicator* comm, Message* msg, TransferOrder* order, BalanceState* balanceState, BalanceHistory* balanceHistory);

local_id get_proc_num_from_args(char* argv[]);
int get_balance_from_args(int lid, char* argv[]);
local_id create_children(pid_t* children, int num);

int balance;

int main(int argc, char* argv[])
{
    pid_t* children;
    local_id procsNum = 6;
    Communicator* comm;

    if (argc > 1)
        procsNum = get_proc_num_from_args(argv) + 1;
    else
    {
        printf("Usage: ./a.out -p X y1 y2 ... yX\n");
        return -1;
    }

    if (procsNum == 1)
    {
        printf("Please provide at least one server!\n");
        return -2;
    }
    // allocate space for children buffer
    children = malloc(sizeof(int) * (procsNum - 1));

    // init log
    log_init();
    // log start of parent process
    log_start(PARENT_ID, 0);

    // generate all pipes between all processes
    comm = generate_communications(procsNum);

    // create children and
    // set pipe communication and free unused fds
    set_communications_local(comm, create_children(children, procsNum - 1));
    if (comm->curr_id != PARENT_ID)
        free(children);
    log_pipes(comm);

    if (comm->curr_id == PARENT_ID)
        do_parent_job(comm);
    else
        do_child_job(comm, argv);

    // if parent, wait for children
    if (comm->curr_id == PARENT_ID)
    {
        for (int i = 0; i < procsNum - 1; ++i)
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
    increment_lamport_time();
    receive_all_msgs(comm, STARTED);

    // PAYLOAD:
    bank_robbery(comm, comm->total_ids - 1);

    increment_lamport_time();
    broadcast_stop_msg(comm);

    receive_all_msgs(comm, DONE);

    AllHistory allHistory;
    allHistory.s_history_len = 0;

    // receive BalanceHistory from every child
    for (local_id i = 1; i < comm->total_ids; ++i)
    {
        Message msg;
        int ret;

        while ((ret = receive_any(comm, &msg)) <= 0);
        if (ret < 0)
        {
            perror("Something went very wrong !asd\n");
            exit(-4);
        }
        if (msg.s_header.s_type != BALANCE_HISTORY)
        { 
            perror("Wrong message type received. Were waiting for BALANCE_HISTORY\n");
            exit(-3);
        }
        BalanceHistory historyEntry;
        
        memcpy((void*)&historyEntry, msg.s_payload, sizeof(char) * msg.s_header.s_payload_len);
        allHistory.s_history[allHistory.s_history_len++] = historyEntry;
    }

    print_history(&allHistory);
}

void update_history(Communicator* comm, BalanceHistory* history, BalanceState* state, balance_t sum, timestamp_t msg_timestamp, char incr, char fixTime)
{
    static timestamp_t prevTime = 0;
    timestamp_t currTime = get_lamport_time() < msg_timestamp ? msg_timestamp : get_lamport_time();
    if (incr != 0)
        ++currTime;
    set_lamport_time(currTime);

    if (fixTime)
        --msg_timestamp;

    // bring basic history up to date
    history->s_history_len = currTime + 1;
    for (timestamp_t i = prevTime; i < currTime; ++i)
    {
        state->s_time = i;
        
        history->s_history[i] = *state;
    }

    // bring pending up to date
    if (sum > 0)
    {
        for (timestamp_t i = msg_timestamp ; i < currTime; ++i)
        {
            history->s_history[i].s_balance_pending_in += sum;
        }
    }

    prevTime = currTime;
    state->s_time = currTime;
    state->s_balance += sum;
    history->s_history[currTime] = *state;
}

void do_child_job(Communicator* comm, char* argv[])
{
    BalanceState balanceState;
    BalanceHistory balanceHistory;

    balanceHistory.s_id = comm->curr_id;

    balanceState.s_balance = get_balance_from_args(comm->curr_id, argv);
    balanceState.s_balance_pending_in = 0;
    balanceState.s_time = 0;

    printf("Child created. Physical time: %d\n", 0);

    update_history(comm, &balanceHistory, &balanceState, 0, 0, 0, 0);

    // send started msg if not parent
    increment_lamport_time();
    broadcast_started_msg(comm, balanceState.s_balance);

    // receive all started msgs from all other processes
    increment_lamport_time();
    receive_all_msgs(comm, STARTED);

    // PAYLOAD:
    // main loop
    while(1) 
    {
        Message msg;
        
        // increment_lamport_time();
        while (comm_receive_any(comm, &msg) <= 0);

        // update_history(comm, &balanceHistory, &balanceState, 0);

        if (msg.s_header.s_type == STOP)
        {
            update_history(comm, &balanceHistory, &balanceState, 0, msg.s_header.s_local_time, 1, 0);
            printf("%d: received STOP msg. Broadcasting DONE\n", comm->curr_id);
            broadcast_done_msg(comm, balanceState.s_balance);
            break;
        }
        else if (msg.s_header.s_type == TRANSFER)
        {
            TransferOrder order;
            memcpy((void*)&order, msg.s_payload, sizeof(char) * msg.s_header.s_payload_len);
            do_transfer(comm, &msg, &order, &balanceState, &balanceHistory);
        }
        else
        {
            fprintf(stderr, "%d: unknown message type? %d\n", comm->curr_id, msg.s_header.s_type);
            exit(-7);
        }
    }

    update_history(comm, &balanceHistory, &balanceState, 0, 0, 1, 0);

    // third phase: receive TRANSFER or DONE
    int donesLeft = comm->total_ids - 2;
    while (donesLeft > 0)
    {
        Message msg;

        while (comm_receive_any(comm, &msg) <= 0);

        //update_history(comm, &balanceHistory, &balanceState, 0);

        if (msg.s_header.s_type == DONE)
        {
            update_history(comm, &balanceHistory, &balanceState, 0, msg.s_header.s_local_time, 1, 0);
            --donesLeft;
            continue;
        }
        
        if (msg.s_header.s_type == TRANSFER)
        {
            TransferOrder order;
            memcpy(&order, msg.s_payload, sizeof(char) * msg.s_header.s_payload_len);
            do_transfer(comm, &msg, &order, &balanceState, &balanceHistory);
        }
    }

    
    // fourth phase: send history
    update_history(comm, &balanceHistory, &balanceState, 0, 0, 1, 0);
    printf("%d: sending BALANCE_HISTORY. Length: %d\n", comm->curr_id, balanceHistory.s_history_len);
    send_balance_history(comm, PARENT_ID, &balanceHistory);
}

void do_transfer(Communicator* comm, Message* msg, TransferOrder* order, BalanceState* balanceState, BalanceHistory* balanceHistory)
{
    // accept
    update_history(comm, balanceHistory, balanceState, 0, 0, 1, 0);
    printf("%d: received TRANSFER msg. Src: %d. Dst: %d. Amount: %d\n", comm->curr_id, order->s_src, order->s_dst, order->s_amount);
    if (order->s_src == comm->curr_id)
    {
        //timestamp_t currTime = get_lamport_time();
        printf("%d: subracting %d from balance\n", comm->curr_id, order->s_amount);
        // subract sum from balance and send message to dst
        
        update_history(comm, balanceHistory, balanceState, -order->s_amount, msg->s_header.s_local_time, 1, 0);
        update_history(comm, balanceHistory, balanceState, 0, 0, 1, 0);
        msg->s_header.s_local_time = get_lamport_time();
        // sned msg
        printf("%d: forwarding order msg to %d\n", comm->curr_id, order->s_dst);
        comm_send(comm, order->s_dst, msg);
    }
    else if (order->s_dst == comm->curr_id)
    {
        printf("%d: adding %d to balance\n", comm->curr_id, order->s_amount);
        // add sum to balance and send ACK
        update_history(comm, balanceHistory, balanceState, order->s_amount, msg->s_header.s_local_time, 1, 1);

        //if (get_lamport_time() < msg->s_header.s_local_time)
        //    update_history(comm, balanceHistory,balanceState, order->s_amount, msg->s_header.s_local_time, 1);


        printf("%d: acknowledging\n", comm->curr_id);
        increment_lamport_time();
        send_acknowledge_msg(comm, PARENT_ID);
    }
    else
    {
        fprintf(stderr, "Got offer which we are not in. Src: %d\tDst: %d\n", order->s_src, order->s_dst);
        exit(-8);
    }
}

local_id get_proc_num_from_args(char* argv[])
{
    if (strcmp(argv[1], "-p") != 0)
        return -1;
    return atoi(argv[2]);
}

int get_balance_from_args(int lid, char* argv[])
{
    return atoi(argv[2 + lid]);
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

