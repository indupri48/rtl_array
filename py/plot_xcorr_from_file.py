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
slice_length = 4096
n_rows = int(n_samples / slice_length)

data = np.zeros((n_rows, slice_length * 2 - 1))

for i in range(n_rows):

    sample_slice0 = samples0[i * slice_length : (i + 1) * slice_length]
    sample_slice1 = samples1[i * slice_length : (i + 1) * slice_length]
    xcorr_result = signal.correlate(sample_slice0, sample_slice1)
    xcorr_result = np.abs(xcorr_result)
    # xcorr_result = 10 * np.log10(xcorr_result)

    data[i] = xcorr_result

left = -slice_length
right = slice_length
bottom = n_samples / SAMPLE_RATE
top = 0
plt.imshow(data, aspect="auto", interpolation='none')#, vmin=-120, vmax=0, extent=(left, right, bottom, top))

plt.xlabel("Sample Offset")
plt.ylabel("Time")
plt.savefig("xcorr_result_noise_off.png")
plt.show()
