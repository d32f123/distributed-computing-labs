#include "communicator.h"
#include "logger.h"
#include "pa2345.h"
#include "banking-common.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE (1024)
char buffer[BUFFER_SIZE];

int set_nonblock(int pipeId)
{
    int flags = fcntl(pipeId, F_GETFL);
    if (flags == -1)
    {
        return errno;
    }
    flags = fcntl(pipeId, F_SETFL, flags | O_NONBLOCK);
    if (flags == -1)
    {
        return errno;
    }
    return 0;
}

Communicator* generate_communications(local_id processNum)
{
    Communicator* this = malloc(sizeof(Communicator));
    int i, j, err_code, offset = (processNum - 1);
    this->all_pipes = malloc(sizeof(int) * (processNum - 1) * processNum * 2);
    this->total_ids = processNum;
    for (i = 0; i < processNum; ++i)
    {
        for (j = 0; j < processNum; ++j)
        {
            int temp_pipes[2];
            if (i == j)
                continue;
            err_code = pipe(temp_pipes);
            if (err_code < 0)
                return (Communicator*)NULL;
            if (set_nonblock(temp_pipes[0]) != 0 || set_nonblock(temp_pipes[1]) != 0)
            {
                perror("Failed to set pipes to O_NONBLOCK");
                exit(-10);
            }

            this->all_pipes[i * offset * 2 + (j > i ? j - 1 : j) * 2 + WRITE_PIPE] = temp_pipes[1];
            this->all_pipes[j * offset * 2 + (i > j ? i - 1 : i) * 2 + READ_PIPE] = temp_pipes[0];
        }
    }
    this->curr_id = 0;
    return this;
}

void set_communications_local(Communicator* self, local_id curr_id)
{
    int offset = self->total_ids - 1;
    self->curr_id = curr_id;

    // copy the pipes that we will need later
    self->pipes = malloc(sizeof(int) * (self->total_ids - 1) * 2);
    memcpy(self->pipes, self->all_pipes + curr_id * offset * 2, sizeof(int) * offset * 2);

    // close all unused pipes
    for (int i = 0; i < self->total_ids; ++i)
    {
        if (i == self->curr_id)
            continue;
        for (int j = 0; j < self->total_ids - 1; ++j)
        {
            close(self->all_pipes[i * offset * 2 + j * 2 + READ_PIPE]);
            close(self->all_pipes[i * offset * 2 + j * 2 + WRITE_PIPE]);
        }
    }

    free(self->all_pipes);
}

void destroy_communicatons(Communicator* self)
{
    for (int i = 0; i < self->total_ids - 1; ++i)
    {
        close(self->pipes[i * 2 + READ_PIPE]);
        close(self->pipes[i * 2 + WRITE_PIPE]);
    }
}


int comm_send(Communicator* comm, local_id dst, const Message * msg)
{
    return send(comm, dst, msg);
}

int comm_send_multicast(Communicator* comm, const Message * msg)
{
    return send_multicast(comm, msg);
}

int comm_receive(Communicator* comm, local_id from, Message * msg, int blocking)
{
    // change input type to non-blocking
    // int flags = 0;
    if (!blocking)
    {
        /*int flags = fcntl(comm->pipes[from * 2 + READ_PIPE], F_GETFL);
        if (flags == -1)
            return errno;

        err_code = fcntl(comm->pipes[from * 2 + READ_PIPE], F_SETFL, flags | O_NONBLOCK);
        if (err_code == -1)
            return errno;*/
    }

    int resp = receive(comm, from, msg);
    
    if (!blocking)
    {
        /*// put the O_NONBLOCK flag back
        flags = fcntl(this->pipes[i * 2 + READ_PIPE], F_GETFL);
        if (flags == -1)
            return errno;

        err_code = fcntl(this->pipes[i * 2 + READ_PIPE], F_SETFL, flags & ~O_NONBLOCK);
        if (err_code == -1)
            return errno;
            */
    }

    return resp;
}

int comm_receive_any(Communicator* comm, Message * msg)
{
    return receive_any(comm, msg);
}


void broadcast_started_msg(Communicator* comm, balance_t balance)
{
    log_start(comm->curr_id, balance);
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = STARTED;
    msg.s_header.s_local_time = get_lamport_time();

    // set string
    int length = snprintf(buffer, BUFFER_SIZE, log_started_fmt, get_lamport_time(), comm->curr_id, getpid(), getppid(), balance);
    if (length <= 0)
    {
        perror("failed to apply snprintf");
        exit(-2);
    }
    msg.s_header.s_payload_len = length;
    memcpy(msg.s_payload, buffer, sizeof(char) * length);

    printf("%d: Broadcasting STARTED msg\n", comm->curr_id);
    send_multicast(comm, &msg);
}

void broadcast_stop_msg(Communicator* comm)
{
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = STOP;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = 0;
    printf("%d: Broadcasting STOP msg\n", comm->curr_id);

    send_multicast(comm, &msg);
}

void send_transfer_msg(Communicator* comm, local_id dst, TransferOrder* order)
{
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = sizeof(TransferOrder);
    printf("%d: Sending TRANSFER to %d\n", comm->curr_id, dst);

    memcpy(msg.s_payload, order, msg.s_header.s_payload_len);
    send(comm, dst, &msg);
}

void send_balance_history(Communicator* comm, local_id dst, BalanceHistory* balanceHistory)
{
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = sizeof(local_id) + sizeof(uint8_t) + sizeof(BalanceState) * balanceHistory->s_history_len;
    printf("%d: Sending balance history to %d\n\tmsg.s_header.s_payload_len = %d\n\tmsg.s_header.s_type = %d\n",
        comm->curr_id, dst, msg.s_header.s_payload_len, msg.s_header.s_type);

    memcpy(msg.s_payload, balanceHistory, msg.s_header.s_payload_len);

    send(comm, dst, &msg);
}

void send_acknowledge_msg(Communicator* comm, local_id dst)
{
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = ACK;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = 0;
    printf("%d: Sending acknowledge msg to %d\n", comm->curr_id, dst);

    send(comm, dst, &msg);
}

void broadcast_done_msg(Communicator* comm, balance_t balance)
{
    log_done(comm->curr_id, balance);
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = 0;
    msg.s_header.s_type = DONE;
    msg.s_header.s_local_time = get_lamport_time();
    
    // set string
    int length = snprintf(buffer, BUFFER_SIZE, log_done_fmt, get_lamport_time(), comm->curr_id, balance);
    if (length <= 0)
    {
        perror("failed to apply snprintf");
        exit(-2);
    }
    msg.s_header.s_payload_len = length;
    memcpy(msg.s_payload, buffer, sizeof(char) * length);

    printf("%d: Broadcasting DONE msg\n", comm->curr_id);
    send_multicast(comm, &msg);
}

void receive_all_from_range(Communicator* comm, local_id start, local_id end, MessageType msgType)
{
    char receivedArr[MAX_PROCESS_ID + 1];
    memset(receivedArr, 0, sizeof(char) * (MAX_PROCESS_ID + 1));
    receivedArr[comm->curr_id] = 1;

    local_id receivedLeft = end - start;
    if (comm->curr_id >= start && comm->curr_id < end)
        --receivedLeft;

    while (receivedLeft > 0)
    {
        for (local_id i = start; i < end; ++i)
        {
            if (receivedArr[i] != 0)
                continue;
            
            Message msg;
            int ret = receive(comm, i, &msg);
            if (ret < 0)
                continue;
            
            receivedArr[i] = 1;
            --receivedLeft;

            GET_AND_SET_LAMPORT(&msg);

            if (msgType == DONE && ret == 0)
                continue;
            else if (ret == 0 && msgType != DONE)
            {
                perror("ERROR: RECEIVED WRONG MESSAGE FROM PIPE, PROCESS HAS ALREADY CLOSED\n");
            }
            else if (msgType != msg.s_header.s_type)
            {
                perror("ERROR: RECEIVED WRONG MESSAGE FROM PIPE\n");
            }
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

void receive_all_msgs(Communicator* comm, MessageType msgType)
{
    receive_all_from_range(comm, 1, comm->total_ids, msgType);
    /*
    Message msg;
    int ret;
    int i;

    for (i = 1; i < comm->total_ids; ++i)
    {
        // skip yourself
        if (i == comm->curr_id)
            continue;
        
        while ((ret = receive_any(comm, &msg)) < 0);
        // the thing about pipes is that if the writing process has already closed, 
        // reading from pipe even if there is something still in there
        // will result in EOL.
        // Therefore, we could assume that the process has sent us a DONE signal
        // and then closed
        if (msgType == DONE && ret == 0)
            continue;
        else if (ret == 0 && msgType != DONE)
        {
            perror("ERROR: RECEIVED WRONG MESSAGE FROM PIPE, PROCESS HAS ALREADY CLOSED\n");
        }
        else if (msgType != msg.s_header.s_type)
        {
            perror("ERROR: RECEIVED WRONG MESSAGE FROM PIPE\n");
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
    }*/
}
