#ifndef LOGGER_H
#define LOGGER_H

#include "includes.h"
#include "communicator.h"

void log_init();
void log_start(local_id lid);
void log_received_all_started(local_id lid);
void log_done(local_id lid);
void log_received_all_done(local_id lid);
void log_pipes(Communicator* comm);
void log_destruct();

#endif
