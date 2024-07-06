from radio import Radio
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import time

radio = Radio(103.7e6, 2.4e6)
radio.begin_streaming()

n_rows = 64
n_samples = 1024
data0 = np.zeros((n_rows, n_samples))
data1 = np.zeros((n_rows, n_samples))

for i in range(n_rows):

    samples0, samples1 = radio.read_samples(n_samples)

    fft_result = np.fft.fftshift(np.fft.fft(samples0))
    fft_result = np.abs(fft_result)
    fft_result = 10.0 * np.log10(fft_result)
    data0[i] = fft_result

    fft_result = np.fft.fftshift(np.fft.fft(samples1))
    fft_result = np.abs(fft_result)
    fft_result = 10.0 * np.log10(fft_result)
    data1[i] = fft_result

    print(f"Recording... {i+1} / {n_rows}")

    time.sleep(0.1)

fig, (ax1, ax2) = plt.subplots(1, 2)

ax1.imshow(data0, aspect="auto", interpolation='none')
ax2.imshow(data1, aspect="auto", interpolation='none')

plt.show()

radio.stop_streaming()