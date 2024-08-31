#include <fftw3.h>
#include "dsp.h"

int get_offset(uint8_t *samples0, uint8_t *samples1, int N) {

    // FFT samples0
    fftw_complex *in0, *out0;
    fftw_plan p0;
        
    in0 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * 2);
    out0 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * 2);
    p0 = fftw_plan_dft_1d(N * 2, in0, out0, FFTW_FORWARD, FFTW_ESTIMATE);
    
    for(int i = 0; i < N; i++) {

        in0[i][0] = (((double)*(samples0 + i * 2)) - 127.5) / 127.5; // samples0 real
        in0[i][1] = (((double)*(samples0 + i * 2 + 1)) - 127.5) / 127.5; // samples0 imag

    }
    for(int i = 0; i < N; i++) {

        in0[i + N][0] = 0;
        in0[i + N][1] = 0;

    }

    fftw_execute(p0); /* repeat as needed */
        
    fftw_destroy_plan(p0);
    fftw_free(in0);

    // FFT samples1
    fftw_complex *in1, *out1;
    fftw_plan p1;
        
    in1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * 2);
    out1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * 2);
    p1 = fftw_plan_dft_1d(N * 2, in1, out1, FFTW_FORWARD, FFTW_ESTIMATE);
        
    for(int i = 0; i < N; i++) {

        in1[i][0] = (((double)*(samples1 + i * 2)) - 127.5) / 127.5; // samples1 real
        in1[i][1] = (((double)*(samples1 + i * 2 + 1)) - 127.5) / 127.5; // samples1 imag
            
    }
    for(int i = 0; i < N; i++) {

        in1[i + N][0] = 0;
        in1[i + N][1] = 0;

    }

    fftw_execute(p1); /* repeat as needed */
        
    fftw_destroy_plan(p1);
    fftw_free(in1);

    fftw_complex *in, *out;
    fftw_plan p;
        
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * 2);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * 2);
    p = fftw_plan_dft_1d(N * 2, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
        
    // Multiply
    for(int i = 0; i < N * 2; i++) {

        in[i][0] = out0[i][0] * out1[i][0] + out0[i][1] * out1[i][1];
        in[i][1] = out1[i][0] * out0[i][1] - out0[i][0] * out1[i][1];
    }

    fftw_execute(p); /* repeat as needed */
        
    fftw_destroy_plan(p);
    fftw_free(in);
FILE *f;
    f = fopen("fft.dat", "w");
    // Magnitude squared
    double max = 0;
    int max_i = 0;
    for(int i = 0; i < N * 2; i++) {

        double mag_squared = out[i][0] * out[i][0] + out[i][1] * out[i][1];

        if(mag_squared > max) {
            max = mag_squared;
            max_i = i;
        }

        fprintf(f, "%f ", mag_squared);

    }
    fclose(f);

    free(out);
    free(out0);
    free(out1);

    return (max_i + N) % (2 * N) - N;

}