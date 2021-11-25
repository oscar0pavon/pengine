#ifndef ENGINE_THREADS
#define ENGINE_THREADS

#include "array.h"
#include <pthread.h>

typedef unsigned long int PEThreadID;

typedef struct PEThread{
		PEThreadID id; 
    Array commands;
    char name[20];
		bool wait;
}PEThread;

typedef enum PEThreadCommandType{ 
	POINTER,
	TEXT
}PEThreadCommandType;

typedef struct PEThreadCommand{
    bool done;
    void* data;
    char command_text[1000];
    void (*command)(void*);
	PEThreadCommandType type;
}PEThreadCommand;

void thread_new_function(void*(*function)(void*),void* argument);
void thread_new_detached(void*(*function)(void*),void* argument,const char* name,PEThreadID*);

void pe_thread_control(Array* thread_commads);

void pe_th_wait(PEThread* thread);

void pe_th_exec_in(PEThreadID to_id , void(*func)(void*), void* argment);

PEThreadID pe_th_render_id;
PEThreadID pe_th_main_id;
PEThreadID pe_th_audio_id;

PEThread thread_main;

#endif // !ENGINE_THREADS
