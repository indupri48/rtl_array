import numpy as np
import matplotlib.pyplot as plt
import scipy.signal as signal

lag = 1000
N_SAMPLES = 4096
samples = np.fromfile("samples.dat", dtype=np.int8, count=N_SAMPLES * 2 * 2)
samplesA = samples[0::4] + 1j * samples[1::4]
samplesB = samples[2::4] + 1j * samples[3::4]

xcorr_py = np.abs(signal.correlate(samplesB, np.pad(samplesA, (lag, lag), mode="constant"), mode="valid"))

xcorr = np.fromfile("xcorr.dat", dtype=np.float32) / 10000000

plt.plot(xcorr)
plt.plot(xcorr_py)
plt.show()