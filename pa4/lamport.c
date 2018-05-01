#include "communicator.h"
#include "lamport.h"

#include <stdio.h>
#include <stdlib.h>

///----LAMPORT QUEUE------

int node_compare(Node* node1, Node* node2);

LamportQueue* init_lamport_queue()
{
    LamportQueue* queue = (LamportQueue*)malloc(sizeof(LamportQueue));

    queue->head_ = NULL;
    queue->tail_ = NULL;

    return queue;
}

void destroy_lamport_queue(LamportQueue* queue)
{
    if (queue->head_ == NULL)
        return;

    Node* curr = queue->head_;
    do
    {
        Node* next = curr->right;
        free(curr);
        curr = next;
    } while (curr != NULL);

    free(queue);
}

void lamport_queue_push(LamportQueue* queue, timestamp_t time, local_id pid)
{
    Node* node = (Node*)malloc(sizeof(Node));
    node->key = time;
    node->value = pid;
    node->left = NULL;
    node->right = NULL;

    if (queue->head_ == NULL)
    {
        queue->head_ = node;
        queue->tail_ = node;
        return;
    }

    if (node_compare(node, queue->head_) < 0)
    {
        node->right = queue->head_;
        queue->head_->left = node;

        queue->head_ = node;
        return;
    }

    if (node_compare(node, queue->tail_) > 0)
    {
        node->left = queue->tail_;
        queue->tail_->right = node;

        queue->tail_ = node;
        return;
    }

    Node* left = queue->head_;
    Node* right = left->right;
    while (!(node_compare(node, left) > 0 && node_compare(node, right) < 0))
    {
        left = right;
        right = left->right;
    }

    node->left = left;
    node->right = right;

    left->right = node;
    right->left = node;
}

int lamport_queue_peek(LamportQueue* queue)
{
  if (queue->head_ == NULL)
    return 0;

  return queue->head_->value;
}

int lamport_queue_pop(LamportQueue* queue)
{
  if (queue->head_ == NULL)
    return 0;

  int ret = queue->head_->value;

  Node* newHead = queue->head_->right;

  if (newHead != NULL)
    newHead->left = NULL;

  free(queue->head_);

  queue->head_ = newHead;

  return ret;
}

char lamport_queue_empty(LamportQueue* queue)
{
  return queue->head_ == NULL;
}

int node_compare(Node* node1, Node* node2)
{
  if (node1->key < node2->key)
    return -1;
  if (node1->key > node2->key)
    return 1;
  if (node1->value < node2->value)
    return -1;
  if (node1->value > node2->value)
    return 1;
  return 0;
}

///----LAMPORT QUEUE END---

///----LAMPORT TIME--------
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
///----LAMPORT TIME END----

