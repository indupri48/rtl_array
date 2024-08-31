#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#include <rtl-sdr.h>

// Receiver settings
const int CENTER_FREQUENCY = 103700000;
const int SAMPLE_RATE = 2400000;
const int GAIN = -10;

// Server
const int PORT = 45565;

// Dongle ring buffer
const int CHUNK_SIZE = 262144;
const int DONGLE_BUFFER_NCHUNKS = 8;

struct rtl_dongle *dongles = NULL;

// Output stream
enum StreamType {
    STDOUT,
    LOCALFILE,
    NETWORK
};
enum StreamType iqDestination;
FILE *output_file = NULL;
struct tcp_server *tcp_server = NULL;

pthread_barrier_t dongle_start_barrier;

pthread_mutex_t chunk_written_mutex;
pthread_cond_t chunk_written_cond;

int do_exit = 0;

struct rtl_dongle {

    int id;
    rtlsdr_dev_t *device_handle;

    uint8_t *ring_buffer;
    int buffer_write_counter;
    int buffer_read_counter;

    pthread_t *worker_thread;

};

struct tcp_server {

    int port;
    int socket_handle;

    struct sockaddr_in client_address;
    int client_socket; 

    // int running;
    // pthread_t acceptor_thread;

};

void read_async_callback(unsigned char *raw_bytes, uint32_t len, void *ctx)
{
    // Retrieve the dongle structure from the context
    struct rtl_dongle *dongle = ctx;

    // Is the program due to stop? If so, do not continue
    if(do_exit) {
        return;
    }

    if(len != CHUNK_SIZE) {
        fprintf(stderr, "Failed to read correct number of samples!");
    }

    int write_index = dongle->buffer_write_counter % DONGLE_BUFFER_NCHUNKS;
    memcpy(dongle->ring_buffer + write_index * CHUNK_SIZE, raw_bytes, len);
    dongle->buffer_write_counter++;
    
    pthread_cond_signal(&chunk_written_cond);

}

static void *dongle_worker(void *context) {

    struct rtl_dongle *dongle = context;

    pthread_barrier_wait(&dongle_start_barrier);

    rtlsdr_read_async(dongle->device_handle, read_async_callback, (void *)dongle, 0, CHUNK_SIZE);

}

int initialise_dongle(struct rtl_dongle *dongle, int id) {

    dongle->id = id;
    
    dongle->ring_buffer = malloc(CHUNK_SIZE * DONGLE_BUFFER_NCHUNKS);
    dongle->buffer_write_counter = 0;
    dongle->buffer_read_counter = 0;

    // Open up the dongle
    rtlsdr_dev_t *device_handle = NULL;
    int err = rtlsdr_open(&device_handle, dongle->id);
    if(err != 0) {
        printf("Failed to initialise dongle with id %i\n", id);
        return 1;
    }

    // Configure dongle
    //rtlsdr_set_dithering(device_handle, 0); // disable dithering
    rtlsdr_set_sample_rate(device_handle, SAMPLE_RATE);
	rtlsdr_set_center_freq(device_handle, CENTER_FREQUENCY);
	rtlsdr_set_tuner_gain(device_handle, GAIN);
	rtlsdr_reset_buffer(device_handle);
    //rtlsdr_set_agc_mode(device_handle, 1);
    dongle->device_handle = device_handle;

    pthread_t thread;
    dongle->worker_thread = &thread;
    pthread_create(dongle->worker_thread, NULL, dongle_worker, dongle);

    return 0;

}

int destroy_dongle(struct rtl_dongle *dongle) {

    rtlsdr_cancel_async(dongle->device_handle);
    
    free(dongle->ring_buffer);

}

int start_tcp_server(struct tcp_server **tcp_server, int port) {

    struct tcp_server *tmp_server = malloc(sizeof(struct tcp_server));

    tmp_server->port = 45565;

    // Create an internet socket and check for errors
    tmp_server->socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (tmp_server->socket_handle < 0) {
		perror("Failed to create a TCP socket\n");
		return 1;
	}

     /* Applying socket options*/
    int true_value = 1;
	if (setsockopt(tmp_server->socket_handle,SOL_SOCKET,SO_REUSEADDR,&true_value,sizeof(int)) == -1) {
	    printf("Setting socket options failed\n");
		return -1;
	}

    // Bind the local address and check for errors
    struct sockaddr_in address;
	address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
	address.sin_port = htons(tmp_server->port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
		perror("Invalid bind address\n");
		return 1;
	}

	if (bind(tmp_server->socket_handle, (struct sockaddr*) &address, sizeof(address)) < 0) {
		perror("Failed to bind the server to an address\n");
		return 1;
	}

    // Tell the server to start listening for clients
    if (listen(tmp_server->socket_handle, 3) < 0) {
		perror("Failed to start listening on the server\n");
		return 1;
	}

    *tcp_server = tmp_server;

	return 0;

}

void interrupt_handler(int signum) {
    
    fprintf(stderr, "\nSignal caught, exiting!\n");

    if(iqDestination == NETWORK) {
        close(tcp_server->socket_handle);
    }

    do_exit = 1;

}

int main(int argc, char *argv[]) {

    // TODO add argument handling for easy selection of IQ destination

    iqDestination = NETWORK;

    char *filename = NULL;

    if(argc == 2){
        filename = argv[1];
    } else {
        filename = "samples.dat";
    }

    // Register the signal interrupt '^C' handler function
    signal(SIGINT, interrupt_handler);
    signal(SIGPIPE, SIG_IGN);

     // Print splash
    printf("\n");
    printf("######################\n");
    printf("# rtl_array by M7JDK #\n");
    printf("######################\n");
    printf("\n");

    // Count the number of dongles connected
    int n_dongles = rtlsdr_get_device_count();
    if(n_dongles == 0) {
        fprintf(stderr, "No RTL-SDR dongles detected!\n");
        return 0;
    }
    printf("Detected %i RTL dongles:\n", n_dongles);
    for(int i = 0; i < n_dongles; i++) {
        // List each dongle
        char manufacturer[256];
        char product[256];
        char serial[256];
        rtlsdr_get_device_usb_strings(0, manufacturer, product, serial);
        printf("> Dongle ID: %i, %s %s, SN: %s\n", i, manufacturer, product, serial);
    }
    printf("\n");

    // Initialise mutex and condition variables
    pthread_barrier_init(&dongle_start_barrier, NULL, 2);
    pthread_mutex_init(&chunk_written_mutex, NULL);
    pthread_cond_init(&chunk_written_cond, NULL);
    // pthread_mutex_init(&frame_buffer_mutex, NULL);

    // Initialise and configure the dongles
    dongles = malloc(n_dongles * sizeof(*dongles));
    struct rtl_dongle *dongle0;
    struct rtl_dongle *dongle1;
    int return_code = 0;
    for(int i = 0; i < n_dongles; i++) {
        return_code += initialise_dongle(&dongles[i], i);
    }
    if(return_code != 0) {
        fprintf(stderr, "Failed to initialize at least one dongle\n");
        return 1;
    }
    usleep(100000); // sleep for 0.1 second to allow each dongle to reach the common barrier
    
    if(iqDestination == NETWORK) {
        // Stream the IQ samples over a TCP connection
        start_tcp_server(&tcp_server, 45565);
        printf("TCP server started successfully\n");
    } else if(iqDestination == LOCALFILE) {
        // Save the IQ samples to a file instead
        output_file = fopen(filename, "wb");
        if(output_file == NULL) {
            printf("Failed to open output file\n");
            return 1;
        }
    }
 
    if(iqDestination == NETWORK) {
        printf("Waiting for a client to connect...\n");
        int addrlen = sizeof(struct sockaddr_in);
        if ((tcp_server->client_socket = accept(tcp_server->socket_handle, (struct sockaddr*) &tcp_server->client_address, (socklen_t*) &addrlen)) < 0) {
            // An error has occurred. Most likely the server socket has been closed
            printf("Client accept error\n");
            return 1;
        }

        printf("Client connected to IQ Server successfully!\n");
        
    }

    // Start the main program operation
    usleep(100000); // TODO see if necessary
    printf("Running...\n");

    while(!do_exit) {

        // Wait until signalled by a ring buffer write operation from one of the dongles
        struct timespec waitUntil;
        struct timeval now;
        gettimeofday(&now, NULL);
        waitUntil.tv_sec = now.tv_sec + 1; // timeout is 1 second currently. Reduce this
        waitUntil.tv_nsec = now.tv_usec * 1000UL;

        int r = pthread_cond_timedwait(&chunk_written_cond, &chunk_written_mutex, &waitUntil);   
        if(r != 0) {
            do_exit = 1;
            break;
        }

        // Check if both buffers have been updated since last loop
        int dongles_written = 0;
        for(int i = 0; i < n_dongles; i++) {
            dongles_written += dongles[i].buffer_write_counter > dongles[i].buffer_read_counter;
        }
        if(dongles_written != n_dongles) {
            // Continue waiting
            continue;
        }            

        // All dongles are ready to be read

        // Create the IQ Frame
        uint8_t *iq_frame = malloc(n_dongles * CHUNK_SIZE);
        for(int i = 0; i < n_dongles; i++) {
            int dongle_read_index = dongles[i].buffer_read_counter % DONGLE_BUFFER_NCHUNKS;
            uint8_t chunk[CHUNK_SIZE];
            memcpy(chunk, dongles[i].ring_buffer + dongle_read_index * CHUNK_SIZE, CHUNK_SIZE);
            int overrange = 0;
            for(int j = 0; j < CHUNK_SIZE; j++) {
                uint8_t I_or_Q_sample = chunk[j];
                if(I_or_Q_sample == 0 || I_or_Q_sample == 255) {
                    printf("ADC overrange!\n");
                    break;
                }
            }
            memcpy(iq_frame + i * CHUNK_SIZE, &chunk, CHUNK_SIZE);
            dongles[i].buffer_read_counter++;
        }

        // Interleave each dongle's IQ data like I0 Q0 I1 Q1 etc.
        int output_buffer_size = n_dongles * CHUNK_SIZE;
        uint8_t output_buffer[output_buffer_size];
        for(int iq_sample_number = 0; iq_sample_number < CHUNK_SIZE / 2; iq_sample_number++) {
            for(int dongle_number = 0; dongle_number < n_dongles; dongle_number++) {
                uint8_t I = *(iq_frame + dongle_number * CHUNK_SIZE + iq_sample_number * 2);
                uint8_t Q = *(iq_frame + dongle_number * CHUNK_SIZE + iq_sample_number * 2 + 1);
                int i = iq_sample_number * n_dongles * 2 + dongle_number * 2;
                output_buffer[i] = I;
                output_buffer[i + 1] = Q;
            }
        }

        free(iq_frame);

        if(iqDestination == NETWORK) {
            int b = send(tcp_server->client_socket, &output_buffer, output_buffer_size, 0);
            printf("%i\n", b);
            if(b == -1) {
                printf("Client disconnected!\n");
                break;
            }
        } else if(iqDestination == LOCALFILE) {
            // Write the IQ frame to the output file stream
            fwrite(&output_buffer, 1, output_buffer_size, output_file);
        } else if(iqDestination == STDOUT) {
            // Write the IQ frame to the standard output
            fwrite(&output_buffer, 1, output_buffer_size, stdout);
        }

    }

    if(iqDestination == NETWORK) {
        free(tcp_server);
    } else if(iqDestination == LOCALFILE) {
        fclose(output_file);
    }

    // Destroy the dongles
    for(int i = 0; i < n_dongles; i++){
        destroy_dongle(&dongles[i]);
    }
    free(dongles);

    printf("\nStopped rtl_array!\n");

}