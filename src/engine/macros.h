#ifndef PE_MACROS
#define PE_MACROS
#include <string.h>
//Set to zero
#define ZERO(s) memset(&s,0,sizeof(s))
#define FOR(s) for(int i = 0; i < s ; i++)

#define rgb(f)(float){f/255.f}

#endif
