#include "communicator.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

Communicator* generate_communications(local_id processNum)
{
    Communicator* this = malloc(sizeof(Communicator));
    int i, j, err_code, offset = (processNum - 1);
    this->all_pipes = malloc(sizeof(int) * (processNum - 1) * processNum * 2);
    this->total_ids = processNum;
    for (i = 0; i < processNum; ++i)
    {
        for (j = 0; j < processNum; ++j)
        {
            int temp_pipes[2];
            if (i == j)
                continue;
            err_code = pipe(temp_pipes);
            if (err_code < 0)
                return (Communicator*)NULL;
            
            this->all_pipes[i * offset * 2 + (j > i ? j - 1 : j) * 2 + WRITE_PIPE] = temp_pipes[1];
            this->all_pipes[j * offset * 2 + (i > j ? i - 1 : i) * 2 + READ_PIPE] = temp_pipes[0];
        }
    }
    this->curr_id = 0;
    return this;
}

void set_communications_local(Communicator* self, local_id curr_id)
{
    int offset = self->total_ids - 1;
    self->curr_id = curr_id;

    // copy the pipes that we will need later
    self->pipes = malloc(sizeof(int) * (self->total_ids - 1) * 2);
    memcpy(self->pipes, self->all_pipes + curr_id * offset * 2, sizeof(int) * offset * 2);

    // close all unused pipes
    for (int i = 0; i < self->total_ids; ++i)
    {
        if (i == self->curr_id)
            continue;
        for (int j = 0; j < self->total_ids - 1; ++j)
        {
            close(self->all_pipes[i * offset * 2 + j * 2 + READ_PIPE]);
            close(self->all_pipes[i * offset * 2 + j * 2 + WRITE_PIPE]);
        }
    }

    free(self->all_pipes);
}

void destroy_communicatons(Communicator* self)
{
    for (int i = 0; i < self->total_ids - 1; ++i)
    {
        close(self->pipes[i * 2 + READ_PIPE]);
        close(self->pipes[i * 2 + WRITE_PIPE]);
    }
}
