#include "threads.h"
#include <engine/macros.h>

#include "log.h"
#include <string.h>
#include <engine/engine.h>

#include <unistd.h>

void thread_new_function(void*(*function)(void*), void* argument){
    pthread_t new_thread_id;
    pthread_create(&new_thread_id,NULL,function,argument);
    /* int result = pthread_detach(new_thread_id);
    if(result != 0){
        LOG("ERROR detaching thread\n");
    } */
}

void thread_new_detached(void*(*function)(void*), void* argument, const char* name, PEThreadID* id){
    pthread_t new_thread_id;
    pthread_create(&new_thread_id,NULL,function,argument);
#ifdef LINUX
    pthread_setname_np(new_thread_id,name);
#endif
    int result = pthread_detach(new_thread_id);
    if(result != 0){
        LOG("ERROR detaching thread\n");
    }
		memcpy(id,&new_thread_id,sizeof(PEThreadID));

}

void pe_thread_control(Array* thread_commads){
		
		if(thread_commads->block)
				return;

		for (u8 i = 0; i < thread_commads->count; i++)
		{
				PEThreadCommand* new_command = array_get(thread_commads,i);
				if(new_command && !new_command->done){
					switch (new_command->type) {
						case FUNCTION:
							new_command->command_function();
							break;
						case POINTER:
							new_command->command(new_command->data);//Execute command
							break;
						case TEXT:
							new_command->command(new_command->command_text);//Execute command
							break;
					}
						new_command->done = true;
				}
		}
		int undone_jobs = 0;
		for (u8 i = 0; i < thread_commads->count; i++)
		{
				PEThreadCommand* new_command = array_get(thread_commads,i);
				if(new_command && !new_command->done){
						undone_jobs ++;
				}
		}
		
		if(undone_jobs == 0)
				array_clean(thread_commads);
}

void pe_th_exec_function(PEThreadID id , void(*function)() ){

	PEThreadID curren_thread_id = pthread_self();

	if(curren_thread_id == id){
		if(function!=NULL)	
			function();
		return;
	}

	PEThreadCommand command;
	ZERO(command);
	command.type = FUNCTION;
	command.command_function = function;;

	if(id == pe_th_render_id){
		array_add(&render_thread_commads,&command);	
		LOG("exec in renderen therd\n");
	}


}

void pe_th_exec_in_with_type(PEThreadID to_id , void(*func)(void*), void* argment, PEThreadCommandType type){
	
	PEThreadID curren_thread_id = pthread_self();

	if(curren_thread_id == to_id){
		func(argment);
		return;
	}

	PEThreadCommand command;
	ZERO(command);
	command.type = type;
	command.command = func;	
	command.data = argment;

	if(to_id == pe_th_render_id){
		array_add(&render_thread_commads,&command);	
		LOG("exec in renderen therd\n");
	}

}
void pe_th_exec_in(PEThreadID to_id , void(*func)(void*), void* argment){
	
	PEThreadID curren_thread_id = pthread_self();

	if(curren_thread_id == to_id){
		func(argment);
		return;
	}

	PEThreadCommand command;
	ZERO(command);
	command.type = POINTER;
	command.command = func;	
	command.data = argment;

	if(to_id == pe_th_render_id){
		array_add(&render_thread_commads,&command);	
		LOG("exec in renderen therd\n");
	}
}

void pe_th_wait(PEThread* thread){
	while(thread->wait){
		sleep(1);			
	}

}
