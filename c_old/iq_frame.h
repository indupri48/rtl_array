struct iq_frame {

    uint8_t *samples0;
    uint8_t *samples1;

    pthread_mutex_t ready_lock;
    pthread_cond_t ready_cond;

};