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
#include <math.h>

#include "dongle.h"
#include "socket.h"

// Receiver settings
const int CENTER_FREQUENCY = 103700000;
const int SAMPLE_RATE = 2400000;
const int GAIN = 240;

// Server
const int PORT = 45565;

// Output stream
enum StreamType {
    STDOUT,
    LOCALFILE,
    NETWORK
};
enum StreamType iqDestination;
FILE *output_file = NULL;
struct tcp_server *server = NULL;

pthread_barrier_t dongle_start_barrier;

pthread_mutex_t samples_read_mutex;
pthread_cond_t samples_read_cond;

int do_exit = 0;

float calculate_mean(float *data, int N) {
    float total = 0.0;
    for(int i = 0; i < N; i++) {
        total += data[i];
    }
    return total / N;
}

void wait_until_n_chunks_written(struct rtl_dongle **dongles, int n_dongles, int n_chunks, pthread_cond_t *samples_read_cond, pthread_mutex_t *samples_read_mutex) {

    while(!do_exit) {

        // Wait until a dongle signals it has recieved new samples
        struct timespec waitUntil;
        struct timeval now;
        gettimeofday(&now, NULL);
        waitUntil.tv_sec = now.tv_sec + 1; // timeout is 1 second currently. Reduce this
        waitUntil.tv_nsec = now.tv_usec * 1000UL;
        int r = pthread_cond_timedwait(samples_read_cond, samples_read_mutex, &waitUntil);   
        if(r != 0) {
            // We have waited for too long and timed out. Stop the program
            do_exit = 1;
            return;
        }
        
        // Check if all dongles have samples to read
        int n_dongles_ready = 0;
        for(int i = 0; i < n_dongles; i++) {
            n_dongles_ready += get_n_chunks_written(dongles[i]) >= n_chunks;
        }
        if(n_dongles_ready != n_dongles) {
            // Continue waiting
            continue;
        }

        // Dongles are ready to be read from
        return;
        
    }

}

int cross_correlate(unsigned char *samplesA, unsigned char *samplesB, int n_samples, int n_lead, int n_lag) {

    float *samplesA_real = malloc(n_samples * sizeof(float));
    float *samplesA_imag = malloc(n_samples * sizeof(float));
    float *samplesB_real = malloc(n_samples * sizeof(float));
    float *samplesB_imag = malloc(n_samples * sizeof(float));

    // Convert byte samples into float data just for this function
    for(int i = 0; i < n_samples; i++) {
        samplesA_real[i] = (float)samplesA[i * 2 + 0];
        samplesA_imag[i] = (float)samplesA[i * 2 + 1];
        samplesB_real[i] = (float)samplesB[i * 2 + 0];
        samplesB_imag[i] = (float)samplesB[i * 2 + 1];
        //printf("%.2f\n", samplesA_real[i]);
    }

    // De-mean the data
    float samplesA_mean_real = calculate_mean(samplesA_real, n_samples);
    float samplesA_mean_imag = calculate_mean(samplesA_imag, n_samples);
    float samplesB_mean_real = calculate_mean(samplesB_real, n_samples);
    float samplesB_mean_imag = calculate_mean(samplesB_imag, n_samples);
    for(int i = 0; i < n_samples; i++) {
        samplesA_real[i] -= samplesA_mean_real;
        samplesA_imag[i] -= samplesA_mean_imag;
        samplesB_real[i] -= samplesB_mean_real;
        samplesB_imag[i] -= samplesB_mean_imag;
    }

    float max_val = 0.0;
    int max_val_index = 0;

    FILE *f = fopen("xcorr.dat", "w");

    for(int i = -n_lead; i <= n_lag; i++) {

        float inner_product_real = 0.0;
        float inner_product_imag = 0.0;

        for(int sample = 0; sample < n_samples; sample++) {
            
            int indexA = sample;
            int indexB = sample - i;

            if(indexB < 0 || indexB >= n_samples) continue;

            inner_product_real += samplesA_real[indexA] * samplesB_real[indexB] + samplesA_imag[indexA] * samplesB_imag[indexB];
            inner_product_imag += samplesA_real[indexA] * samplesB_imag[indexB] - samplesA_imag[indexA] * samplesB_real[indexB];
            
        }

        float mag_squared = inner_product_real * inner_product_real + inner_product_imag * inner_product_imag;
        // printf("%.2f,\n", mag_squared);
        if(mag_squared > max_val) {
            max_val = mag_squared;
            max_val_index = i + n_lead;
        }
        fwrite(&mag_squared, sizeof(float), 1, f);

    }
    fclose(f);

    free(samplesA_real);
    free(samplesA_imag);
    free(samplesB_real);
    free(samplesB_imag);

    // f = fopen("samples2.dat", "w");
    // for(int i = 0; i < n_samples;i++) {
    //     fwrite(samplesA, sizeof(unsigned char), 2, f);
    //     fwrite(samplesB, sizeof(unsigned char), 2, f);
    // }
    // fclose(f);

    return max_val_index - n_lead;

}

void find_channel_offsets(struct rtl_dongle **dongles, int n_dongles, int *offsets, int max_offset) {

    unsigned char *samples[n_dongles];
    for(int i = 0; i < n_dongles; i++) {
        samples[i] = malloc(READ_CHUNK_LENGTH * sizeof(unsigned char) * 2);
        read_chunk_from_dongle(dongles[i], samples[i]);
    }
    
    for(int i = 1; i < n_dongles; i++) {
        int offset = cross_correlate(samples[0], samples[i], READ_CHUNK_LENGTH, max_offset, max_offset);
        printf(" Dongle %i offset from dongle 0: %i\n", i, offset);
        offsets[i] = offset;
    }
    
    for(int i = 0; i < n_dongles; i++) {
        free(samples[i]);
    }

}

void synchronise_channels(struct rtl_dongle **dongles, int n_dongles) {

    // Calculate the channel offsets
    printf("\nCalculating channel offsets...\n");
    int offsets[n_dongles];
    for(int i = 0; i < 1; i++) {
        wait_until_n_chunks_written(dongles, n_dongles, 4, &samples_read_cond, &samples_read_mutex);
        find_channel_offsets(dongles, n_dongles, offsets, 1000);
    }

    // Find the greatest lag
    int greatest_lag = 0;
    for(int i = 0; i < n_dongles; i++) {
        if(offsets[i] < greatest_lag) {
            greatest_lag = offsets[i];
        }
    }

    // Apply the correct delay to each channel
    printf("Synchronising channnels using channel offsets:\n");
    for(int i = 0; i < n_dongles; i++) {
        printf("%i ", offsets[i] - greatest_lag);
        set_delay(dongles[i], offsets[i] - greatest_lag);
    }
    printf("\nDone!\n");

}

void interrupt_handler(int signum) {
    
    // The user has called ^C. Stop the program

    printf("\nSignal caught, exiting!\n");

    if(iqDestination == NETWORK) {
        close_server(server);
    }

    do_exit = 1;

}

int main(int argc, char *argv[]) {

    iqDestination = LOCALFILE;

    char *filename = NULL;

    if(argc == 2){
        filename = argv[1];
    } else {
        filename = "samples.dat";
    }

    // Assign a callback function to the user interrupt signal
    signal(SIGINT, interrupt_handler);
    signal(SIGPIPE, SIG_IGN);

     // Print splash
    printf("\n");
    printf("######################\n");
    printf("# rtl_array by M7JDK #\n");
    printf("######################\n");
    printf("\n");

    // Initialise mutex and condition variables
    pthread_barrier_init(&dongle_start_barrier, NULL, 2);
    pthread_mutex_init(&samples_read_mutex, NULL);
    pthread_cond_init(&samples_read_cond, NULL);

    // Initialise all of the dongles
    int n_dongles = list_dongle_devices();
    struct rtl_dongle *dongles[n_dongles];
    for(int i = 0; i < n_dongles; i++) {
        create_dongle(&dongles[i], i, &samples_read_cond, &dongle_start_barrier);
        set_centre_frequency(dongles[i], 88600000);
        set_sample_rate(dongles[i], 2400000);
        set_gain(dongles[i], 0);
        start_dongle(dongles[i]);
    }

    usleep(100000); // allow time for all dongles to reach the starting barrier

    synchronise_channels(dongles, n_dongles);

    if(iqDestination == NETWORK) {
        // Stream the IQ samples over a TCP connection
        start_server(&server, PORT);
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
        accept_client(server);
        printf("Client connected to IQ Server successfully!\n");
    }

    // Start the main program operation
    printf("\nRunning...\n");

    int iq_frame_size = n_dongles * (READ_CHUNK_LENGTH * 2) * sizeof(unsigned char);
    unsigned char *iq_frame = malloc(iq_frame_size); // 2 for I and Q
    unsigned char *chunk_buffer = malloc(READ_CHUNK_LENGTH * 2 * sizeof(unsigned char)); // necessary for interleaving

    while(!do_exit) {

        wait_until_n_chunks_written(dongles, n_dongles, 1, &samples_read_cond, &samples_read_mutex);       
        
        // All dongles are now ready to be read from

        // Populate the iq_frame with channels interleaved
        for(int i = 0; i < n_dongles; i++) {
            read_chunk_from_dongle(dongles[i], chunk_buffer);
            for(int iq_sample = 0; iq_sample < READ_CHUNK_LENGTH; iq_sample++) {
                iq_frame[(iq_sample * n_dongles + i) * 2 + 0] = chunk_buffer[iq_sample * 2 + 0]; // I sample
                iq_frame[(iq_sample * n_dongles + i) * 2 + 1] = chunk_buffer[iq_sample * 2 + 1]; // Q sample
            }
        }

        if(iqDestination == NETWORK) {
            int bytes_sent = send_bytes(server, iq_frame, iq_frame_size);
            if(bytes_sent != iq_frame_size) {
                printf("Client disconnected!\n");
                do_exit = 1;
                break;
            }
        } else if(iqDestination == LOCALFILE) {
            // Write the IQ frame to the output file stream
            fwrite(iq_frame, sizeof(unsigned char), iq_frame_size, output_file);
        } else if(iqDestination == STDOUT) {
            // Write the IQ frame to the standard output
            fwrite(iq_frame, 1, iq_frame_size, stdout);
        }

    }

    free(iq_frame);

    if(iqDestination == NETWORK) {
        close_server(server);
    } else if(iqDestination == LOCALFILE) {
        fclose(output_file);
    }

    // Destroy the dongles
    for(int i = 0; i < n_dongles; i++){
        destroy_dongle(dongles[i]);
    }

    printf("\nStopped rtl_array!\n");

}