#ifndef LOGGER_H
#define LOGGER_H

#include "ipc.h"
#include "lamport.h"
#include "communicator.h"
#include "common.h"
#include "pa2345.h"

void log_init();
void log_destruct();

void log_start(local_id lid);
void log_received_all_started(local_id lid);
void log_done(local_id lid);
void log_received_all_done(local_id lid);

void log_pipes(Communicator* comm);


#endif
