#ifndef PE_LINUX_NETWORK_H
#define PE_LINUX_NETWORK_H

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <arpa/inet.h>


typedef struct NetworkConnecion NetworkConnecion;

void network_print_recieve_data(NetworkConnecion*);
void network_create_server(NetworkConnecion*);

void pe_network_connect(NetworkConnecion*);
void pe_net_send_message(NetworkConnecion*,const char* message);
#endif
