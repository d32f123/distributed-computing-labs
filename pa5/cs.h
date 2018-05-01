#ifndef CS_H
#define CS_H

#include "ipc.h"
#include "communicator.h"
#include "lamport.h"

extern int donesLeft;

void cs_messages_logic(Communicator* comm, Message* msg);

#endif

