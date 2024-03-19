import socket
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt

fft_length = 1024
n_rows = 1000
n_samples = fft_length * n_rows
n_channels = 2

samples = np.fromfile("samples.dat", dtype=np.uint8, count=n_samples * n_channels * 2)
samplesA = samples[0::n_channels*2] + 1j * samples[1::n_channels*2]

data = np.zeros((n_rows, fft_length))
for i in range(n_rows):
    
    fft_result = np.fft.fft(samplesA[i * fft_length : (i + 1) * fft_length])
    power_spectrum = np.abs(fft_result)
    power_spectrum = np.fft.fftshift(power_spectrum)
    power_spectrum = 10.0 * np.log10(power_spectrum)
    data[i,:] = power_spectrum
    
plt.imshow(data)
plt.show()