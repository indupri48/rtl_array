#include <sys/socket.h>
#include <arpa/inet.h>


// ================ CLIENT ================

struct tcp_client {
    int port;
    int fd;
};

// Create a client socket and connect it to a server at hostname:port.
// The client will attempt to connect to the server 3 times before 
// giving up and returning 1. A successful attempt returns 0.
int start_client(struct tcp_client **client, char *hostname, int port, int max_n_attempts);

int close_client(struct tcp_client *client);


// ================ SERVER ================

struct tcp_server {
    int port;
    int fd;

    // Client informtion
    struct sockaddr_in client_address;
    int client_fd; 
};

int start_server(struct tcp_server **server, int port);

int accept_client(struct tcp_server *server);

int close_server(struct tcp_server *server);


// ================ COMMON ================

int receive_bytes(struct tcp_client *client, char *in_buffer, int n_bytes);

int send_bytes(struct tcp_server *server, char *out_buffer, int n_bytes);
