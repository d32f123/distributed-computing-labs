#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "ipc.h"
#include "lamport.h"

enum PipeType 
{
    READ_PIPE = 0,
    WRITE_PIPE = 1
};

typedef struct 
{
    int* all_pipes;
    int* pipes;         // array should be of size: X - 1 (contains descriptors for 
                        // communicating with everyone except oneself)
                        // therefore, pipes[i], where i < curr_id 
                        // means descriptors to communicate with proc i
                        // and if i >= curr_id
                        // means descriptors to communicate with proc (i + 1) 
    local_id curr_id;
    local_id total_ids; // number of processes (including oneself)

    local_id last_message_from;
} Communicator;

Communicator* generate_communications(local_id processNum);
void set_communications_local(Communicator* self, local_id curr_id);
void destroy_communicatons(Communicator* self);

int broadcast_started_msg(Communicator* comm);
int broadcast_done_msg(Communicator* comm);
int broadcast_stop_msg(Communicator* comm);
void receive_all_msgs(Communicator* comm, MessageType msgType);
void receive_all_from_range(Communicator* comm, local_id start, local_id end, MessageType msgType);

int send_reply_msg(Communicator* comm, local_id pid);
int broadcast_request_msg(Communicator* comm);
int broadcast_release_msg(Communicator* comm);

#endif
