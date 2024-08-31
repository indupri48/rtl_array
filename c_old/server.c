#include "server.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void* acceptor_worker(void *arg) {

    struct tcp_server *server = (struct tcp_server*) arg;
    struct rtl_dongle *dongle0 = server->dongle0;
    struct rtl_dongle *dongle1 = server->dongle1;

	struct sockaddr_in address;

    while(server->running) {

        int client_socket;
		int addrlen = sizeof(address);

		if ((client_socket = accept(server->socket_handle, (struct sockaddr*) &address, (socklen_t*) &addrlen)) < 0) {
			printf("err\n");
            break;
		}

        printf("Client connected to IQ Server successfully!")

        while(1) {

            s

        // int n_iq_samples = 4096;
        // int n_bytes = n_iq_samples * 2 * 2;

        // uint8_t *samples0 = malloc(n_iq_samples * 2);
        // uint8_t *samples1 = malloc(n_iq_samples * 2);

        // pthread_mutex_lock(dongle0->lock);
        // memcpy(samples0, dongle0->buffer + dongle0->delay * 2, n_iq_samples * 2);
        // memcpy(samples1, dongle1->buffer + dongle1->delay * 2, n_iq_samples * 2);
		// pthread_mutex_unlock(dongle0->lock);

        // FILE *f;
    	// f = fopen("samples.dat", "w");
        // uint8_t payload[n_bytes];
        // for(int i = 0; i < n_iq_samples; i++) {
            
        //     uint8_t I0 = *(samples0 + i * 2);
		// 	uint8_t Q0 = *(samples0 + i * 2 + 1);
		// 	uint8_t I1 = *(samples1 + i * 2);
		// 	uint8_t Q1 = *(samples1 + i * 2 + 1);

		// 	payload[i * 4 + 0] = I0;
		// 	payload[i * 4 + 1] = Q0;
		// 	payload[i * 4 + 2] = I1;
		// 	payload[i * 4 + 3] = Q1;

        //     fprintf(f, "%u %u %u %u ", I0, Q0, I1, Q1);

        // }
        // fclose(f);
        // int b = send(client_socket, &payload, sizeof(payload), 0);
		// close(client_socket);

        // free(samples0);
        // free(samples1);

        }
        
    }

}

int start_tcp_server(struct tcp_server **server, int port, struct rtl_dongle *dongle0, struct rtl_dongle *dongle1) {

    // Initialisation of server struct with parameters
    struct tcp_server *tmp_server = malloc(sizeof(struct tcp_server));
    tmp_server->running = 0;
    tmp_server->port = port;
    tmp_server->dongle0 = dongle0;
    tmp_server->dongle1 = dongle1;

    // Create an internet socket and check for errors
    tmp_server->socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (tmp_server->socket_handle < 0) {
		free(tmp_server);
		perror("Failed to create a TCP socket\n");
		return -1;
	}

    // Bind the local address and check for errors
    struct sockaddr_in address;
	address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
	address.sin_port = htons(tmp_server->port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
		free(tmp_server);
		perror("Invalid bind address\n");
		return -1;
	}

	if (bind(tmp_server->socket_handle, (struct sockaddr*) &address, sizeof(address)) < 0) {
		free(tmp_server);
		perror("Failed to bind the server to an address\n");
		return -1;
	}

    // Tell the server to start listening for clients
    if (listen(tmp_server->socket_handle, 3) < 0) {
		free(tmp_server);
		perror("Failed to start listening on the server\n");
		return -1;
	}

    // Start up the client acceptor thread
    pthread_t acceptor_thread;
	int code = pthread_create(&acceptor_thread, NULL, &acceptor_worker, tmp_server);
    if (code != 0) {
        free(tmp_server);
        perror("Failed to start the client acceptor thread\n");
        return -1;
    }
    tmp_server->running = 1;
	tmp_server->acceptor_thread = acceptor_thread;

	*server = tmp_server;
    printf("TCP server started successfully\n");

	return 0;

}

int stop_tcp_server(struct tcp_server *server) {

    server->running = 0;
	int code = shutdown(server->socket_handle, SHUT_RDWR);
    if (code != 0) {
        
    }
    close(server->socket_handle);
    
    printf("TCP server has been stopped successfully");

}
