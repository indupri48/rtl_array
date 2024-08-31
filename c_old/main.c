// #include "server.h"
#include "dsp.h"
#include "dongle.h"
#include "iq_frame.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <pthread.h>
#include <string.h>

int main(int argc, char *argv[]) {

    if(argc != 4) {
        printf(".rtl_array [fc] [fs] [g]\n");
        return 1;
    }

    // Handle input arguments
    int center_frequency = atoi(argv[1]);
    int sampling_rate =    atoi(argv[2]) ;
    int gain = atoi(argv[3]);

    // Print splash
    printf("\n");
    printf("######################\n");
    printf("# rtl_array by M7JDK #\n");
    printf("######################\n");
    printf("\n");

    // Check that there are exactly two rtl dongles connected
    int n_dongles = rtlsdr_get_device_count();
    if(n_dongles != 2) {
        printf("%i RTL dongle(s) detected. Requires exactly 2\n");
        return 1;
    }
    printf("Detected 2 RTL dongles\n");
    
    // Get more information about the two dongles
    char manufacturer[256];
    char product[256];
    char serial[256];
    rtlsdr_get_device_usb_strings(0, manufacturer, product, serial);
    printf("> Dongle ID: 0, %s %s, SN: %s\n", manufacturer, product, serial);
    rtlsdr_get_device_usb_strings(1, manufacturer, product, serial);
    printf("> Dongle ID: 1, %s %s, SN: %s\n", manufacturer, product, serial);
    printf("\n");

    pthread_mutex_t starting_lock;
    pthread_cond_t starting_cond;
    pthread_mutex_init(&starting_lock, NULL);
    pthread_cond_init(&starting_cond, NULL);

    pthread_mutex_t reader_lock;
    pthread_cond_t reader_cond;
    pthread_mutex_init(&reader_lock, NULL);
    pthread_cond_init(&reader_cond, NULL);

    pthread_mutex_t iq_server_lock;
    pthread_cond_t iq_server_cond;
    pthread_mutex_init(&iq_server_lock, NULL);
    pthread_cond_init(&iq_server_cond, NULL);

    // Initialise and configure the two dongles

    struct rtl_dongle *dongle0;
    struct rtl_dongle *dongle1;
    initialise_dongle(&dongle0, 0, &starting_lock, &starting_cond, &reader_lock, &reader_cond, center_frequency, sampling_rate, gain);
    initialise_dongle(&dongle1, 1, &starting_lock, &starting_cond, &reader_lock, &reader_cond, center_frequency, sampling_rate, gain);    
    usleep(100000); // sleep for 0.1 second to allow both dongles to be ready to start

    struct iq_frame iq_frame;
    iq_frame.samples0 = malloc(dongle0->chunk_size);
    iq_frame.samples1 = malloc(dongle1->chunk_size);
    iq_frame.ready_lock = iq_server_lock;
    iq_frame.ready_cond = iq_server_cond;

    // START THE PROGRAM

    // TODO consider using pthreads barrier for this instead - makes more sense
    pthread_mutex_lock(&starting_lock);
    pthread_cond_broadcast(&starting_cond); // start the two dongles
    pthread_mutex_unlock(&starting_lock);
    
    printf("Successfully started the two dongles\n");
    printf("\n");
    
    int buffer_read_counter = 0;

    while(1) {

        pthread_cond_wait(&reader_cond, &reader_lock);

        // Check if both dongles have collected a new chunk of samples each
        int dongle0_ready = dongle0->buffer_write_counter == buffer_read_counter + 1;
        int dongle1_ready = dongle1->buffer_write_counter == buffer_read_counter + 1;

        if (dongle0_ready & dongle1_ready == 0) {
            continue;
        }

        buffer_read_counter++;

        int buffer_read_counter_modulo = buffer_read_counter % dongle0->n_chunks;

        memcpy(dongle0->)

    }

    return 0;
    
}