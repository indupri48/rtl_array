#include <pthread.h>

#define READ_CHUNK_LENGTH 8192
#define BUFFER_LENGTH 10 // in units of chunks

struct rtl_dongle;

int list_dongle_devices();

int create_dongle(struct rtl_dongle **dongle, int id, pthread_cond_t *samples_read_cond, pthread_barrier_t *start_barrier);

void destroy_dongle(struct rtl_dongle *dongle);

void start_dongle(struct rtl_dongle *dongle);

void stop_dongle(struct rtl_dongle *dongle);

int get_n_chunks_written(struct rtl_dongle *dongle);

void read_chunk_from_dongle(struct rtl_dongle *dongle, unsigned char *iq_samples); 

int get_n_chunks_written(struct rtl_dongle *dongle);

void set_delay(struct rtl_dongle *dongle, int delay);

int set_sample_rate(struct rtl_dongle *dongle, int sample_rate_Hz);

int set_centre_frequency(struct rtl_dongle *dongle, int centre_frequency_Hz);

int set_gain(struct rtl_dongle *dongle, int gain_dB);