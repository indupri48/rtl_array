#include <pthread.h>
#include "dongle.h"

struct tcp_server {

    int port;
    int socket_handle;

    int running;
    pthread_t acceptor_thread;

    struct rtl_dongle *dongle0;
	struct rtl_dongle *dongle1;

};

static void* acceptor_worker(void *arg);

// Returns -1 if failed
int start_tcp_server(struct tcp_server **server, int port, struct rtl_dongle *dongle0, struct rtl_dongle *dongle1);

int stop_tcp_server();