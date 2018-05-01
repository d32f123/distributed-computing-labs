#ifndef CS_H
#define CS_H

#include "communicator.h"
#include "lamport.h"

struct CommAndQueue
{
    Communicator* comm;
    LamportQueue* queue;
};

extern int donesLeft;

void cs_messages_logic(Communicator* comm, LamportQueue* queue, Message* msg);

#endif

