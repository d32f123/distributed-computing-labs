#include "logger.h"

#include "stdarg.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

FILE* pipes_log_file;
FILE* events_log_file;

void log_init()
{
    pipes_log_file = fopen(pipes_log, "w");
    events_log_file = fopen(events_log, "w+");
}

void log_start(local_id lid, balance_t balance)
{
    fprintf(stdout, log_started_fmt, get_lamport_time(), lid, getpid(), getppid(), balance);
    fprintf(events_log_file, log_started_fmt, get_lamport_time(), lid, getpid(), getppid(), balance);
}

void log_received_all_started(local_id lid)
{
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), lid);
    fprintf(events_log_file, log_received_all_started_fmt, get_lamport_time(), lid);
}

void log_done(local_id lid, balance_t balance)
{
    fprintf(stdout, log_done_fmt, get_lamport_time(), lid, balance);
    fprintf(events_log_file, log_done_fmt, get_lamport_time(), lid, balance);
}

void log_received_all_done(local_id lid)
{
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), lid);
    fprintf(events_log_file, log_received_all_done_fmt, get_lamport_time(), lid);
}

void log_transfer_out(local_id src, balance_t sum, local_id dst) 
{
    fprintf(stdout, log_transfer_out_fmt, get_lamport_time(), src, sum, dst);
    fprintf(events_log_file, log_transfer_out_fmt, get_lamport_time(), src, sum, dst);
}

void log_transfer_in(local_id src, balance_t sum, local_id dst)
{
    fprintf(stdout, log_transfer_in_fmt, get_lamport_time(), dst, sum, src);
    fprintf(events_log_file, log_transfer_in_fmt, get_lamport_time(), dst, sum, src);
}


void log_pipes(Communicator* comm)
{
    int i;

    fprintf(stdout, "Process %1d pipes: ", comm->curr_id);
    fprintf(pipes_log_file, "Process %1d pipes: ", comm->curr_id);
    for (i = 0; i < comm->total_ids; ++i)
    {
        if (i == comm->curr_id)
            continue;
        fprintf(stdout, "pr%1d|R%d|W%d ", i, comm->pipes[(i < comm->curr_id ? i : i - 1) * 2 + READ_PIPE], 
                                                comm->pipes[(i < comm->curr_id ? i : i - 1) * 2 + WRITE_PIPE]);
        fprintf(pipes_log_file, "pr%1d|R%d|W%d ", i, comm->pipes[(i < comm->curr_id ? i : i - 1) * 2 + READ_PIPE], 
                                                comm->pipes[(i < comm->curr_id ? i : i - 1) * 2 + WRITE_PIPE]);
    }
    fprintf(stdout, "\n");
    fprintf(pipes_log_file, "\n");
}

void log_destruct()
{
    fclose(pipes_log_file);
    fclose(events_log_file);
}
