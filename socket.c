#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "socket.h"


// ================ CLIENT ================

int start_client(struct tcp_client **client, char *hostname, int port, int max_n_attempts) {

    struct tcp_client *tmp_client = malloc(sizeof(struct tcp_client));
	
	// Set the server parameters
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(hostname);
	server_address.sin_port = htons(port);
	
	int n_attempts = 0;
	while(n_attempts < max_n_attempts || max_n_attempts == -1) {

		// Note: a new client socket must be created for each attempt

		// Create the client socket
		tmp_client->fd = socket(AF_INET, SOCK_STREAM, 0);
		if (tmp_client->fd == -1) return 1;

		// Connect to the server
		int ret = connect(tmp_client->fd, (struct sockaddr*)&server_address, sizeof(server_address));
		if(ret == 0) {
			// Able to connect
			*client = tmp_client;
			return 0;
		}

		// Failed to connect on this attempt
		n_attempts++;
		usleep(1000000); // TODO skip on last attempt

	}
	
	// Failed to connect after the maximum number of attempts
	free(tmp_client);
	return 1;

}

int close_client(struct tcp_client *client) {

	// TODO add error handling

    close(client->fd);
	
    free(client);

	return 0;

}


// ================ SERVER ================

int start_server(struct tcp_server **server, int port) {

    struct tcp_server *tmp_server = malloc(sizeof(struct tcp_server));
    tmp_server->port = port;

	// Create the server socket
    tmp_server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tmp_server->fd < 0) {
		free(tmp_server);
		return 1;
	}

	// Allow the server to reuse the same address. Useful for quick restarts of the program
    int allow_address_reuse = 1;
	int ret = setsockopt(tmp_server->fd, SOL_SOCKET, SO_REUSEADDR, &allow_address_reuse, sizeof(int));
	if(ret != 0) {
		free(tmp_server);
		return 1;
	}

	// Server address and port
    struct sockaddr_in address;
	address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
	address.sin_port = htons(tmp_server->port);

    // Convert IP address from text to binary form
    ret = inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
	if(ret != 1) {
		// Confusingly, 1 is success for this function, not 0
		free(tmp_server);
		return 1;
	}

    // Bind the server socket to the localhost address
	ret = bind(tmp_server->fd, (struct sockaddr*) &address, sizeof(address));
	if(ret != 0) {
		free(tmp_server);
		return 1;
	}

    // Configure the server to start listening for incoming client connections
    ret = listen(tmp_server->fd, 1); // One client connection in the backlog only
	if(ret != 0) {
		free(tmp_server);
		return 1;
	}

	tmp_server->client_fd = -1;
    *server = tmp_server;

	return 0;

}

int accept_client(struct tcp_server *server) {

    int client_address_size = sizeof(struct sockaddr_in);
    server->client_fd = accept(server->fd, (struct sockaddr*) &server->client_address, (socklen_t*) &client_address_size);
	if(server->client_fd < 0) {
		// TODO set client fd back to something neutral?
        return 1;
    }

    return 0;

};

int close_server(struct tcp_server *server) {

    close(server->fd);

    free(server);

    return 0;

}


// ================ COMMON ================

int receive_bytes(struct tcp_client *client, char *in_buffer, int n_bytes) {
	int total_n_bytes_read = 0;
	while(total_n_bytes_read < n_bytes) {
		int ret = read(client->fd, in_buffer + total_n_bytes_read, n_bytes - total_n_bytes_read);
		if(ret <= 0) return ret;
		total_n_bytes_read += ret;
	}
	return total_n_bytes_read;
}

int send_bytes(struct tcp_server *server, char *out_buffer, int n_bytes) {
	int ret = send(server->client_fd, out_buffer, n_bytes, 0);
	if(ret != n_bytes) {
		return ret;
	}
	return n_bytes;
}