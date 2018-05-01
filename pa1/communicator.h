#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "includes.h"

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
} Communicator;

Communicator* generate_communications(local_id processNum);
void set_communications_local(Communicator* self, local_id curr_id);
void destroy_communicatons(Communicator* self);

// TODO: FUNCTIONS TO GENERATE PIPES FOR ALL PROCESSES
// TODO: FUNCTION TO CLOSE UNUSED PIPES FOR CHILD PROCCESS

#endif
