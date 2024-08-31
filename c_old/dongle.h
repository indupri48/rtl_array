#include <rtl-sdr.h>
#include <pthread.h>

struct rtl_dongle {

    int id; // uniquely assigned to each dongle
    rtlsdr_dev_t *device_handle; // needed for referring to the dongle

    int chunk_size;
    int n_chunks;
    int ring_buffer_size; // length of the sample buffer. length in bytes
    uint8_t *ring_buffer; // sample buffer with each I or Q sample being a byte
    int buffer_write_counter;

    // Threading and sequencing

    pthread_t *thread; // thread ID used for dongle_worker
    pthread_mutex_t *starting_lock;
    pthread_cond_t *starting_cond;
    
    pthread_mutex_t *reader_lock;
    pthread_cond_t *reader_cond;

};

static void *dongle_worker(void *arg);

int initialise_dongle(struct rtl_dongle **dongle, int id, pthread_mutex_t *starting_lock, pthread_cond_t *starting_cond, pthread_mutex_t *reader_lock, pthread_cond_t *reader_cond, int center_freq, int sample_rate, int gain);

int destroy_dongle(struct rtl_dongle *dongle);