#include "cs.h"

#include <stdio.h>
#include <stdlib.h>

#include "lamport.h"
#include "communicator.h"
#include "ipc.h"

void cs_messages_logic(Communicator* comm, LamportQueue* queue, Message* msg)
{
    if (msg->s_header.s_type == DONE)
        --donesLeft;
    else if (msg->s_header.s_type == CS_REQUEST)
    {
        lamport_queue_push(queue, msg->s_header.s_local_time, comm->last_message_from);

        increment_lamport_time();
        send_reply_msg(comm, comm->last_message_from);
    }
    else if (msg->s_header.s_type == CS_RELEASE)
    {
        if (lamport_queue_pop(queue) != comm->last_message_from)
        {
            perror("LAMPORT QUEUE HEAD IS NOT THE SAME AS RELEASE MSG RECEIVED");
            exit(-12);
        }
    }
}

int request_cs(const void * self)
{
    struct CommAndQueue* commAndQueue = (struct CommAndQueue*) self;
    LamportQueue* queue = commAndQueue->queue;
    Communicator* comm = commAndQueue->comm;

    Message msg;

    while (receive_any(comm, &msg) > 0) 
    {
        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, queue, &msg);
    }

    increment_lamport_time();
    lamport_queue_push(queue, get_lamport_time(), comm->curr_id);
    broadcast_request_msg(comm);

    int responsesLeft = comm->total_ids - 1;
    while (responsesLeft > 0)
    {
        while (receive_any(comm, &msg) <= 0);

        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, queue, &msg);
        if (msg.s_header.s_type == CS_REPLY)
        {
            --responsesLeft;
        }
    }

    while (lamport_queue_peek(queue) != comm->curr_id)
    {
        while (receive_any(comm, &msg) <= 0);

        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, queue, &msg);
    }

    return 0;
}

int release_cs(const void * self)
{
    struct CommAndQueue* commAndQueue = (struct CommAndQueue*) self;
    LamportQueue* queue = commAndQueue->queue;
    Communicator* comm = commAndQueue->comm;

    Message msg;

    while (receive_any(comm, &msg) > 0) 
    {
        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, queue, &msg);
    }

    increment_lamport_time();
    broadcast_release_msg(comm);
    lamport_queue_pop(queue);

    return 0;
}

