#ifndef LAMPORT_H
#define LAMPORT_H

#include "ipc.h"

/* LAMPORT TIME */
void increment_lamport_time();
timestamp_t get_lamport_time();
void set_lamport_time(timestamp_t newLamportTime);

#define GET_AND_SET_LAMPORT(msg) { timestamp_t currTime =  get_lamport_time() < (msg)->s_header.s_local_time ? \
                                                            (msg)->s_header.s_local_time : get_lamport_time(); \
    set_lamport_time(currTime); \
    increment_lamport_time(); }

#endif

