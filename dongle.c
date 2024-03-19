#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rtl-sdr.h>

#include "dongle.h"

struct rtl_dongle {

    int id;
    rtlsdr_dev_t *device_handle;

    // Buffer
    unsigned char *buffer;
    int write_index;
    int read_index;
    int delay;
    
    pthread_t *worker_thread;
    pthread_barrier_t *start_barrier;

    // Signalling
    pthread_cond_t *samples_read;

};

int list_dongle_devices() {

    // Count the number of dongles connected
    int n_dongles = rtlsdr_get_device_count();
    if(n_dongles == 0) {
        printf("No RTL-SDR dongles detected!\n");
        return 0;
    }
    
    // At least one dongle device has been found
    printf("Detected %i RTL dongles:\n", n_dongles);
    for(int i = 0; i < n_dongles; i++) {
        // List each dongle's serialised data
        unsigned char manufacturer[256];
        unsigned char product[256];
        unsigned char serial[256];
        rtlsdr_get_device_usb_strings(i, manufacturer, product, serial);
        printf(" Dongle ID: %i, %s %s, SN: %s\n", i, manufacturer, product, serial);
    }
    printf("\n");

    return n_dongles;

}

void read_async_callback(unsigned char *raw_samples, uint32_t len, void *context) {

    // Note: len is number of bytes in the buffer, not IQ samples (i.e. 2 bytes)

    // TODO determine whether n_sanples_read != READ_CHUNK_LENGTH is okay or not. Assumed not for now

    struct rtl_dongle *dongle = (struct rtl_dongle*)context;
     

    // Check that the expected number of samples have been read
    if(len != READ_CHUNK_LENGTH * 2) {
        fprintf(stderr, "Unexpected number of samples read! Stopping...\n");
        return;
    }

    // Write these samples to the buffer
    int chunk_size = READ_CHUNK_LENGTH * sizeof(unsigned char) * 2;

    memcpy(dongle->buffer + dongle->write_index * READ_CHUNK_LENGTH * 2, raw_samples, chunk_size);

    dongle->write_index = (dongle->write_index + 1) % BUFFER_LENGTH;

    // Signal to the main thread that this dongle has read a chunk of samples
    pthread_cond_signal(dongle->samples_read);
     
}

void *dongle_worker(void *context) {

    struct rtl_dongle *dongle = context;

    // This is crucial to making sure all dongles start reading at a similar time
    pthread_barrier_wait(dongle->start_barrier);

    // This is what drives the asynchronous read operations
    rtlsdr_read_async(dongle->device_handle, read_async_callback, (void *)dongle, 0, READ_CHUNK_LENGTH * 2);

}

int create_dongle(struct rtl_dongle **dongle, int id, pthread_cond_t *samples_read_cond, pthread_barrier_t *start_barrier) {

    struct rtl_dongle *tmp_dongle = malloc(sizeof(struct rtl_dongle));
    tmp_dongle->id = id;
    tmp_dongle->samples_read = samples_read_cond;
    tmp_dongle->start_barrier = start_barrier;

    // Connect to the dongle device
    tmp_dongle->device_handle = NULL;
    int ret = rtlsdr_open(&tmp_dongle->device_handle, id);
    if(ret != 0) {
        fprintf(stderr, "Failed to initialise dongle with ID %i\n", id);
        return 1;
    }
	rtlsdr_reset_buffer(tmp_dongle->device_handle); // for good measure
    
    // Initialise the buffer
    tmp_dongle->write_index = 0;
    tmp_dongle->read_index = 0;
    tmp_dongle->buffer = malloc(BUFFER_LENGTH * READ_CHUNK_LENGTH * sizeof(unsigned char) * 2);
    tmp_dongle->delay = 0;

    tmp_dongle->worker_thread = malloc(sizeof(pthread_t));

    *dongle = tmp_dongle;

    return 0;

}

void destroy_dongle(struct rtl_dongle *dongle) {
    free(dongle->worker_thread);
    free(dongle->buffer);
    free(dongle);
}

void start_dongle(struct rtl_dongle *dongle) {
    pthread_create(dongle->worker_thread, NULL, dongle_worker, dongle);
}

void stop_dongle(struct rtl_dongle *dongle) {
    rtlsdr_cancel_async(dongle->device_handle);
}

void read_chunk_from_dongle(struct rtl_dongle *dongle, unsigned char *iq_samples) {
    // A non-zero delay could mean that the read chunk spans the start/end of the buffer
    // If this is the case, do two read operations, A and B, before and after the break
    if(dongle->read_index == 0 && dongle->delay > 0) {
        int writeA_size = dongle->delay * sizeof(unsigned char) * 2;
        int writeB_size = (READ_CHUNK_LENGTH - dongle->delay) * sizeof(unsigned char) * 2;
        memcpy(iq_samples, dongle->buffer + (BUFFER_LENGTH * READ_CHUNK_LENGTH - dongle->delay) * 2, writeA_size);
        memcpy(iq_samples + dongle->delay * 2, dongle->buffer, writeB_size);
    } else {
        int chunk_size = READ_CHUNK_LENGTH * sizeof(unsigned char) * 2;
        memcpy(iq_samples, dongle->buffer + (dongle->read_index * READ_CHUNK_LENGTH - dongle->delay) * 2, chunk_size);
    }
    dongle->read_index = (dongle->read_index + 1) % BUFFER_LENGTH;
}

int get_n_chunks_written(struct rtl_dongle *dongle) {
    return (dongle->write_index - dongle->read_index + BUFFER_LENGTH) % BUFFER_LENGTH;
}

void set_delay(struct rtl_dongle *dongle, int delay) {
    dongle->delay = delay;
}

int set_sample_rate(struct rtl_dongle *dongle, int sample_rate_Hz) {
    int ret = rtlsdr_set_sample_rate(dongle->device_handle, sample_rate_Hz);
    if(ret != 0) return 1;
    return 0;
}

int set_centre_frequency(struct rtl_dongle *dongle, int centre_frequency_Hz) {
    int ret = rtlsdr_set_center_freq(dongle->device_handle, centre_frequency_Hz);
    if(ret != 0) return 1;
    return 0;
}

int set_gain(struct rtl_dongle *dongle, int gain_dB) {
    int ret = rtlsdr_set_tuner_gain(dongle->device_handle, 420);
    if(ret != 0) return 1;
    return 0;
}