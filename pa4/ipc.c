#include "includes.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

int send(void * self, local_id dst, const Message * msg)
{
    Communicator* this = (Communicator*) self;
    local_id index;
    if (dst == this->curr_id)
        return -1; // cannot send messages to myself

    index = GET_COMM_INDEX(dst, this);
    return write(this->pipes[index * 2 + WRITE_PIPE], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
}

int send_multicast(void * self, const Message * msg)
{
    Communicator* this = (Communicator*) self;
    int err_code, i;
    
    for (i = 0; i < this->total_ids - 1; ++i)
    {
        err_code = write(this->pipes[i * 2 + WRITE_PIPE], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
        if (err_code < 0)
            return err_code;
    }

    return 0;
}

int receive_inner(Communicator* this, local_id index, Message* msg)
{
    int err_code, received = 0;
    err_code = read(this->pipes[index * 2 + READ_PIPE], msg, sizeof(MessageHeader)); // read only the header to check how long is the actual message
    if (err_code < (int)sizeof(MessageHeader))
    {
        return err_code;
    }
    received += err_code;

    err_code = read(this->pipes[index * 2 + READ_PIPE], ((char*) msg) + sizeof(MessageHeader), msg->s_header.s_payload_len);

    if (err_code < 0)
    {
        return err_code;
    }
    received += err_code;

    return received;
}

int receive(void * self, local_id from, Message * msg)
{
    Communicator* this = (Communicator*) self;
    local_id index;

    if (from == this->curr_id)
    {
        perror("TRIED TO RECEIVE MESSAGE FROM THYSELF\n");
        return -1; // cannot receive messages from myself
    }

    index = GET_COMM_INDEX(from, this);
    
    int ret = receive_inner(this, index, msg);
    if (ret > 0)
    {
        this->last_message_from = from;
    }

    return ret;
}

int receive_any(void * self, Message * msg)
{
    Communicator* this = (Communicator*) self;
    int err_code, i;

    for (i = 0; i < this->total_ids - 1; ++i)
    {
        // try to read
        err_code = receive_inner(this, i, msg);
        if (err_code > 0)
        {  
            printf("%d: received ANY message from %d\n", this->curr_id, i >= this->curr_id ? i + 1 : i);
            this->last_message_from = i >= this->curr_id ? i + 1 : i;
            return err_code;
        }
    }
    return 0;
}

