from rtl_array_interface import load_iq_samples_from_file
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import time

N_CHANNELS = 2

CENTER_FREQUENCY = 103.7e6
SAMPLE_RATE = 2.4e6

channel_data = load_iq_samples_from_file("../data/noise_off.dat", 2)
samples0 = channel_data[0]
samples1 = channel_data[1]

n_samples = len(samples0)
fft_length = 4096
n_rows = int(n_samples / fft_length)
plt.plot(samples0.real)
plt.show()
data0 = np.zeros((n_rows, fft_length))
data1 = np.zeros((n_rows, fft_length))

for i in range(n_rows):

    sample_slice = samples0[i * fft_length : (i + 1) * fft_length]
    fft_result = np.fft.fftshift(np.fft.fft(sample_slice))
    fft_result /= n_samples
    fft_result = np.abs(fft_result) ** 2
    fft_result = 10.0 * np.log10(fft_result)
    data0[i] = fft_result
    
    sample_slice = samples1[i * fft_length : (i + 1) * fft_length]
    fft_result = np.fft.fftshift(np.fft.fft(sample_slice))
    fft_result /= n_samples
    fft_result = np.abs(fft_result) ** 2
    fft_result = 10.0 * np.log10(fft_result)
    data1[i] = fft_result

fig, (ax0, ax1) = plt.subplots(1, 2)

left = (CENTER_FREQUENCY - SAMPLE_RATE / 2) / 1e6
right = (CENTER_FREQUENCY + SAMPLE_RATE / 2) / 1e6
bottom = n_samples / SAMPLE_RATE
top = 0
ax0.imshow(data0, aspect="auto", interpolation='none', vmin=-120, vmax=0, extent=(left, right, bottom, top))
plt.ylabel("Time")
cb = ax1.imshow(data0, aspect="auto", interpolation='none', vmin=-120, vmax=0, extent=(left, right, bottom, top))

plt.colorbar(cb)
plt.xlabel("Frequency (MHz)")
plt.savefig("waterfall_file_noise_off.png")
plt.show()
