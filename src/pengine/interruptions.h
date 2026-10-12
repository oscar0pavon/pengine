#ifndef PE_INTERRUPTIONS_H
#define PE_INTERRUPTIONS_H

#include <signal.h>

static inline void debug_break(){
    raise(SIGINT);
}

#endif