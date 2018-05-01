#include "banking.h"
#include "communicator.h"
#include "banking-common.h"

#include <stdio.h>

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount)
{
    Communicator* comm = (Communicator*) parent_data;
    TransferOrder order;
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount; 

    increment_lamport_time();

    printf("%d: TRANSFER src: %d. dst: %d. amount: %d\n", comm->curr_id, src, dst, amount);
    send_transfer_msg(comm, src, &order);
    
    Message msg;
    int ret;
    while ((ret = receive(comm, dst, &msg)) <= 0);
    printf("%d: ACK received: %d. Msg type: %d", comm->curr_id, ret, msg.s_header.s_type);

    GET_AND_SET_LAMPORT(&msg)

    if (msg.s_header.s_type != ACK)
    {
        perror("Waited for acknowlede. got something else\n");
    }
}

static timestamp_t lamport_time = 0;

void increment_lamport_time() {
    ++lamport_time;
}

void set_lamport_time(timestamp_t newLamportTime) {
    if (lamport_time < newLamportTime)
        lamport_time = newLamportTime;
}

timestamp_t get_lamport_time()
{
    return lamport_time;
}


