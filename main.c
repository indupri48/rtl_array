#include "server.h"
#include "dsp.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

int main() {

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

    // initialise mutex stuff
    pthread_mutex_t lock;
    pthread_cond_t cond;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL); 

    // Initialise and configure the two dongles
    int center_frequency = 103700000; // 103.7MHz
    int sampling_rate = 2400000; // 2.4MHz
    int gain = 450; // 45dB

    struct rtl_dongle *dongle0;
    struct rtl_dongle *dongle1;
    initialise_dongle(&dongle0, 0, &lock, &cond, center_frequency, sampling_rate, gain);
    initialise_dongle(&dongle1, 1, &lock, &cond, center_frequency, sampling_rate, gain);    
    usleep(200000); // sleep for 100ms to allow the dongles to get ready

    pthread_mutex_lock(&lock);
    pthread_cond_broadcast(&cond); // start the two dongles
    pthread_mutex_unlock(&lock);
    usleep(2000000); // sleep for 100ms to allow the dongles to get ready

    printf("Successfully started the two dongles\n");
    printf("\n");

    // Detect and correct the channel bulk delays
    int n_measurements = 11;
    int n_iq_samples = 4096;
        
    int offsets[n_measurements];
    for(int i = 0; i < n_measurements; i++) {

        int n_bytes = n_iq_samples * 2 * 2;

        uint8_t *samples0 = malloc(n_iq_samples * 2);
        uint8_t *samples1 = malloc(n_iq_samples * 2);

        pthread_mutex_lock(dongle0->lock);
        memcpy(samples0, dongle0->buffer, n_iq_samples * 2);
        memcpy(samples1, dongle1->buffer, n_iq_samples * 2);
        pthread_mutex_unlock(dongle0->lock);
        
        int offset = get_offset(samples0, samples1, n_iq_samples);
        printf("%i\n", offset);
        offsets[i] = offset;

        usleep(100000);

    }
    int i,j;

    for(i = 0;i < n_measurements-1;i++) {
        for(j = 0;j < n_measurements-i-1;j++) {
            if(offsets[j] > offsets[j+1]) {
                float tmp = offsets[j];
                offsets[j] = offsets[j + 1];
                offsets[j + 1] = tmp;
            }
        }
    }

    int delay = offsets[(n_measurements - 1) / 2];
    
    if(delay < 0) {
        dongle0->delay = 0;
        dongle1->delay = -delay;
    } else {
        dongle0->delay = delay;
        dongle1->delay = 0;
    }

    printf("Estimated channel offset is %i\n", delay);

    printf("\n");

    // Create the TCP server
    struct tcp_server *server = malloc(sizeof(struct tcp_server));
    int err = start_tcp_server(&server, 8080, dongle0, dongle1);
    if(err != 0) {
        printf("Failed to start TCP server\n");
        return 1;
    }
    printf("\n");

    usleep(1000000); // sleep for 1000ms just in case

    printf("Running...\n");

    // Idle the main thread
    while(1) {usleep(100000000);}

    return 0;
    
}