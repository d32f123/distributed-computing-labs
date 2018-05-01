#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "ipc.h"
#include "banking.h"

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
    balance_t balance;
} Communicator;

Communicator* generate_communications(local_id processNum);
void set_communications_local(Communicator* self, local_id curr_id);
void destroy_communicatons(Communicator* self);

int comm_send(Communicator* comm, local_id dst, const Message * msg);
int comm_send_multicast(Communicator* comm, const Message * msg);
int comm_receive(Communicator* comm, local_id from, Message * msg, int blocking);
int comm_receive_any(Communicator* comm, Message * msg);

void send_transfer_msg(Communicator* comm, local_id dst, TransferOrder* order);
void send_balance_history(Communicator* comm, local_id dst, BalanceHistory* balanceHistory);
void send_acknowledge_msg(Communicator* comm, local_id dst);
void broadcast_started_msg(Communicator* comm, balance_t balance);
void broadcast_done_msg(Communicator* comm, balance_t balance);
void broadcast_stop_msg(Communicator* comm);
void receive_all_msgs(Communicator* comm, MessageType msgType);
void receive_all_from_range(Communicator* comm, local_id start, local_id end, MessageType msgType);

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount);

#endif
