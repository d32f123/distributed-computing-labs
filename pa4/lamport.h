#ifndef LAMPORT_H
#define LAMPORT_H

#include "ipc.h"

/* LAMPORT QUEUE */

struct NodeT
{
    struct NodeT* left;
    struct NodeT* right;

    int key;
    int value;
};
typedef struct NodeT Node;

typedef struct LamportQueueT
{
    Node* head_;
    Node* tail_;
} LamportQueue;

LamportQueue* init_lamport_queue();
void destroy_lamport_queue(LamportQueue* queue);

void lamport_queue_push(LamportQueue* queue, timestamp_t time, local_id pid);
int lamport_queue_peek(LamportQueue* queue);
int lamport_queue_pop(LamportQueue* queue);
char lamport_queue_empty(LamportQueue* queue);

/* LAMPORT TIME */
void increment_lamport_time();
timestamp_t get_lamport_time();
void set_lamport_time(timestamp_t newLamportTime);

#define GET_AND_SET_LAMPORT(msg) { timestamp_t currTime =  get_lamport_time() < (msg)->s_header.s_local_time ? \
                                                            (msg)->s_header.s_local_time : get_lamport_time(); \
    set_lamport_time(currTime); \
    increment_lamport_time(); }

#endif

