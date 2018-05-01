#include "communicator.h"
#include "lamport.h"

#include <stdio.h>
#include <stdlib.h>

static timestamp_t lamport_time = 0;

void increment_lamport_time() 
{
    ++lamport_time;
}

void set_lamport_time(timestamp_t newLamportTime) 
{
    if (lamport_time < newLamportTime)
        lamport_time = newLamportTime;
}

timestamp_t get_lamport_time()
{
    return lamport_time;
}

