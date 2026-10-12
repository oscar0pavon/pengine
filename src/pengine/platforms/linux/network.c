#include "../../network/network.h"
#include "../../log.h"
#include "network.h"
#include <string.h>
void network_create_server(struct NetworkConnecion *in_connection) {
	struct NetworkConnecion connection_copy;
	memcpy(&connection_copy,in_connection,sizeof(struct NetworkConnecion));
	
	struct NetworkConnecion* connection = &connection_copy;

	LOG("Creating server: %s \n",connection->name); 

	connection->server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (connection->server_socket < 0) {
    shutdown(connection->server_socket, SHUT_RDWR);
    close(connection->server_socket);
    LOG("ERROR opening socket");
    return;
  }

  if (setsockopt(connection->server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                 sizeof(int)) < 0) {
    LOG("setsockopt(SO_REUSEADDR) failed");
    shutdown(connection->server_socket, SHUT_RDWR);
    close(connection->server_socket);
    return;
  }

  struct sockaddr_in address;
  int addrlen = sizeof(address);

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(connection->port);

  if (bind(connection->server_socket, (struct sockaddr *)&address,
           sizeof(address)) < 0) {
    LOG("Fail to bind socket form connection %s \n", connection->name);
    shutdown(connection->server_in_socket, SHUT_RDWR);
    close(connection->server_socket);
    return;
  }

  listen(connection->server_socket, 3);

  while (connection->server_running == 1) {
    connection->server_in_socket =
    accept(connection->server_socket, (struct sockaddr *)&address,
               (socklen_t *)&addrlen);
    if (connection->server_running != 1)
      return;
    connection->data_process(connection);
  }
  shutdown(connection->server_socket, SHUT_RDWR);
  close(connection->server_in_socket);
	LOG("Finish server: %s \n", connection->name);
}

void network_print_recieve_data(struct NetworkConnecion *connection) {
  char buffer[1024] = {0};
  int readed = read(connection->server_in_socket, buffer, 1024);
  if (readed < 0) {
    shutdown(connection->server_in_socket, SHUT_RDWR);
    close(connection->server_in_socket);
    return;
  }
  printf("%s\n", buffer);
}

void pe_network_connect(struct NetworkConnecion* connection){
  int valread;
  struct sockaddr_in serv_addr;
  char buffer[1024] = {0};
  if ((connection->server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(connection->port);

  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return;
  }

  if (connect(connection->server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return;
  }
  //valread = read(sock,buffer,1024);
  //printf("%s\n",buffer);
}

void pe_net_send_message(struct NetworkConnecion* connection,const char* message){
	
  send(connection->server_socket, message, strlen(message), 0);

}

