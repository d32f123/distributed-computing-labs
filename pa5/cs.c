#include "cs.h"

#include <stdio.h>
#include <stdlib.h>

#include "lamport.h"
#include "communicator.h"
#include "ipc.h"

char dr_array[MAX_PROCESS_ID + 1];

int compare_requests(timestamp_t time1, local_id pid1, timestamp_t time2, local_id pid2);

void cs_messages_logic(Communicator* comm, Message* msg)
{
    switch (msg->s_header.s_type)
    {
        case DONE:
            --donesLeft;
            break;
        case CS_REQUEST:
            increment_lamport_time();
            send_reply_msg(comm, comm->last_message_from);
            break;
        case CS_RELEASE:
            perror("UNEXPECTED CS_RELEASE");
            exit(-12);
    }
}

int request_cs(const void * self)
{
    Communicator* comm = (Communicator*) self;

    Message msg;

    while (receive_any(comm, &msg) > 0) 
    {
        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        cs_messages_logic(comm, &msg);
    }

    increment_lamport_time();
    broadcast_request_msg(comm);
    timestamp_t requestTime = get_lamport_time();

    int responsesLeft = comm->total_ids - 1;
    while (responsesLeft > 0)
    {
        while (receive_any(comm, &msg) <= 0);

        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        switch (msg.s_header.s_type)
        {
            case DONE:
                --donesLeft;
                break;
            case CS_REQUEST:
                {
                    int comp = compare_requests(requestTime, comm->curr_id, msg.s_header.s_local_time, comm->last_message_from);
                    if (comp > 0)
                    {
                        increment_lamport_time();
                        send_reply_msg(comm, comm->last_message_from);
                    }
                    else if (comp < 0)
                    {
                        dr_array[comm->last_message_from] = 1;
                    }
                    else
                    {
                        perror("Request comparison returned 0. That's a deadlock right there!\nExiting\n");
                        exit(-13);
                    }
                    break;
                }
            case CS_REPLY:
                --responsesLeft;
                break;
            default:
                perror("UNEXPECTED MESSAGE TYPE IN REQUEST_CS\n");
                exit(-14);
        }
    }

    return 0;
}

int release_cs(const void * self)
{
    Communicator* comm = (Communicator*) self;

    Message msg;

    while (receive_any(comm, &msg) > 0) 
    {
        set_lamport_time(msg.s_header.s_local_time);
        increment_lamport_time();

        switch (msg.s_header.s_type)
        {
            case DONE:
                --donesLeft;
                break;
            case CS_REQUEST:
                dr_array[comm->last_message_from] = 1;
                break;
            case CS_REPLY:
                perror("UNEXPECTED CS_REPLY IN RELEASE_CS\n");
                exit(-14);
            default:
                perror("UNEXPECTED MESSAGE TYPE IN RELEASE_CS\n");
                exit(-14);
        }
    }

    for (int i = 0; i < MAX_PROCESS_ID + 1; ++i)
    {
        if (dr_array[i])
        {
            increment_lamport_time();
            send_reply_msg(comm, i);
            dr_array[i] = 0;
        }
    }

    return 0;
}

int compare_requests(timestamp_t time1, local_id pid1, timestamp_t time2, local_id pid2)
{
    if (time1 < time2)
        return -1;
    if (time1 > time2)
        return 1;
    if (pid1 < pid2)
        return -1;
    if (pid1 > pid2)
        return 1;
    return 0;
}

