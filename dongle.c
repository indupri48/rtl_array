#include "dongle.h"

#include <stdio.h>
#include <stdlib.h>

void read_async_callback(unsigned char *raw_bytes, uint32_t len, void *ctx)
{
    struct rtl_dongle *dongle = ctx;
    pthread_mutex_lock(dongle->lock);
    dongle->buffer = raw_bytes;
    pthread_mutex_unlock(dongle->lock);

}

static void *dongle_worker(void *arg) {

    struct rtl_dongle *dongle = arg;

    pthread_mutex_lock(dongle->lock);
    pthread_cond_wait(dongle->cond, dongle->lock);
    pthread_mutex_unlock(dongle->lock);
    
    rtlsdr_read_async(dongle->device_handle, read_async_callback, (void *)dongle, 0, dongle->buffer_length * 2);

}

int initialise_dongle(struct rtl_dongle **dongle, int id, pthread_mutex_t *lock, pthread_cond_t *cond, int center_freq, int sample_rate, int gain) {

    struct rtl_dongle *tmp_dongle = malloc(sizeof(struct rtl_dongle));
    tmp_dongle->id = id;
    tmp_dongle->buffer_length = 16384 * 4;
    tmp_dongle->buffer = malloc(tmp_dongle->buffer_length * 2);

    tmp_dongle->lock = lock;
    tmp_dongle->cond = cond;
    
    // Open up the dongle
    rtlsdr_dev_t *device_handle = NULL;
    int err = rtlsdr_open(&device_handle, tmp_dongle->id);
    if(err != 0) {
        printf("Failed to initialise dongle with id %i\n", id);
        return 1;
    }

    // Configure dongle
    rtlsdr_set_dithering(device_handle, 0); // disable dithering
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