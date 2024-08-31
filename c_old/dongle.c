#include "dongle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_async_callback(unsigned char *raw_bytes, uint32_t len, void *ctx)
{

    // Retrieve the dongle structure
    struct rtl_dongle *dongle = ctx;
    
    // pthread_mutex_lock(dongle->lock);
    
    int write_index = dongle->buffer_write_counter % dongle->buffer_write_counter;
    memcpy(dongle->ring_buffer + write_index * dongle->chunk_size, raw_bytes, len);

    dongle->buffer_write_counter++;
    
    // pthread_mutex_unlock(dongle->lock);

    // printf("%i, %i\n", dongle->id, dongle->buffer_write_counter);

    pthread_cond_signal(dongle->reader_cond);

}

static void *dongle_worker(void *context) {

    struct rtl_dongle *dongle = context;

    pthread_mutex_lock(dongle->starting_lock);
    pthread_cond_wait(dongle->starting_cond, dongle->starting_lock);
    pthread_mutex_unlock(dongle->starting_lock);

    rtlsdr_read_async(dongle->device_handle, read_async_callback, (void *)dongle, 0, dongle->chunk_size);

}

int initialise_dongle(struct rtl_dongle **dongle, int id, pthread_mutex_t *starting_lock, pthread_cond_t *starting_cond, pthread_mutex_t *reader_lock, pthread_cond_t *reader_cond, int center_freq, int sample_rate, int gain) {

    struct rtl_dongle *tmp_dongle = malloc(sizeof(struct rtl_dongle));

    tmp_dongle->id = id;
    
    // Data buffering
    tmp_dongle->chunk_size = 262144;
    tmp_dongle->n_chunks = 8;
    tmp_dongle->ring_buffer_size = tmp_dongle->chunk_size * tmp_dongle->n_chunks;
    tmp_dongle->ring_buffer = malloc(tmp_dongle->ring_buffer_size);
    tmp_dongle->buffer_write_counter = 0;

    tmp_dongle->starting_lock = starting_lock;
    tmp_dongle->starting_cond = starting_cond;
    
    tmp_dongle->reader_lock = reader_lock;
    tmp_dongle->reader_cond = reader_cond;

    // Open up the dongle
    rtlsdr_dev_t *device_handle = NULL;
    int err = rtlsdr_open(&device_handle, tmp_dongle->id);
    if(err != 0) {
        printf("Failed to initialise dongle with id %i\n", id);
        return 1;
    }

    // Configure dongle
    //rtlsdr_set_dithering(device_handle, 0); // disable dithering
    rtlsdr_set_sample_rate(device_handle, sample_rate);
	rtlsdr_set_center_freq(device_handle, center_freq);
	rtlsdr_set_tuner_gain(device_handle, gain);
	rtlsdr_reset_buffer(device_handle);
    tmp_dongle->device_handle = device_handle;

    pthread_t thread;
    tmp_dongle->thread = &thread;
    pthread_create(tmp_dongle->thread, NULL, dongle_worker, tmp_dongle);

    *dongle = tmp_dongle;

    return 0;

}

int destroy_dongle(struct rtl_dongle *dongle) {

    rtlsdr_cancel_async(dongle->device_handle);
    
    free(dongle);

}