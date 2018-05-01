#include "includes.h"
#include "communicator.h"
#include "logger.h"

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
    printf("Sending message to %d from %d\n", dst, this->curr_id);
    return write(this->pipes[index * 2 + WRITE_PIPE], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
}

int send_multicast(void * self, const Message * msg)
{
    Communicator* this = (Communicator*) self;
    int err_code, i;
    
    for (i = 0; i < this->total_ids - 1; ++i)
    {
        printf("TO|%1d|SENT|FROM|%1d|%d\n", i >= this->curr_id ? i + 1 : i, this->curr_id, msg->s_header.s_type); 
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
    if (err_code <= sizeof(MessageHeader))
        return err_code;
    received += err_code;

    err_code = read(this->pipes[index * 2 + READ_PIPE], ((char*) msg) + sizeof(MessageHeader), msg->s_header.s_payload_len);

    if (err_code < 0)
        return err_code;
    received += err_code;
    return received;
}

int receive(void * self, local_id from, Message * msg)
{
    Communicator* this = (Communicator*) self;
    local_id index;

    if (from == this->curr_id)
        return -1; // cannot receive messages from myself

    index = GET_COMM_INDEX(from, this);
    
    return receive_inner(this, index, msg);
}

int receive_any(void * self, Message * msg)
{
    Communicator* this = (Communicator*) self;
    int err_code, i;

    for (i = 0; i < this->total_ids - 1; ++i)
    {
        // change input type to non-blocking
        int flags = fcntl(this->pipes[i * 2 + READ_PIPE], F_GETFL);
        if (flags == -1)
            return errno;

        err_code = fcntl(this->pipes[i * 2 + READ_PIPE], F_SETFL, flags | O_NONBLOCK);
        if (err_code == -1)
            return errno;

        // try to read
        err_code = receive_inner(this, i, msg);
        if (err_code > 0)
        {
            printf("TO|%1d|RECV|FROM|%1d|%d\n", this->curr_id, i >= this->curr_id ? i + 1 : i, msg->s_header.s_type);   
            return err_code;
        }
        // try another one if read failed
        // but don't forget to put the O_NONBLOCK flag back
        flags = fcntl(this->pipes[i * 2 + READ_PIPE], F_GETFL);
        if (flags == -1)
            return errno;

        err_code = fcntl(this->pipes[i * 2 + READ_PIPE], F_SETFL, flags & ~O_NONBLOCK);
        if (err_code == -1)
            return errno;
    }
    return 0;
}
