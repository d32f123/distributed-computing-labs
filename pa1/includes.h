#ifndef INCLUDES_H
#define INCLUDES_H

#ifdef LOCAL_WORK
    #include "pa1_starter_code/ipc.h"
    #include "pa1_starter_code/common.h"
    #include "pa1_starter_code/pa1.h"
#else
    #include "ipc.h"
    #include "common.h"
    #include "pa1.h"
#endif

#include <stdlib.h>
#define GET_COMM_INDEX(x, comm) ((x) < (comm)->curr_id ? (x) : (x) - 1)

#endif
