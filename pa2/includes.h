#ifndef INCLUDES_H
#define INCLUDES_H

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "logger.h"
#include "banking.h"
#include "communicator.h"

#include <stdlib.h>
#define GET_COMM_INDEX(x, comm) ((x) < (comm)->curr_id ? (x) : (x) - 1)

#endif
