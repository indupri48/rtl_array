#include <rtl-sdr.h>
#include <pthread.h>

struct rtl_dongle {

    int id; // uniquely assigned to each dongle
    rtlsdr_dev_t *device_handle; // needed for refering to the dongle

    int buffer_length; // length of the sample buffer. length in IQ samples, not bytes
    uint8_t *buffer; // sample buffer with each I or Q sample being a byte
    int delay; // artificial delay added to this dongle to ensure samples are coherent

    pthread_t *thread; // thread ID used for dongle_worker
    pthread_mutex_t *lock; // used to allow only one process to access the buffer at a time
    pthread_cond_t *cond; // used to get the dongles to start within one buffer length

};

static void *dongle_worker(void *arg);

int initialise_dongle(struct rtl_dongle **dongle, int id, pthread_mutex_t *lock, pthread_cond_t *cond, int center_freq, int sample_rate, int gain);

int destroy_dongle(struct rtl_dongle *dongle);