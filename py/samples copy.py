import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt

N_SAMPLES = 1024
samples = np.fromfile("samples.dat", dtype=np.int8, count=N_SAMPLES * 2)
samplesA = samples[0::4] + 1j * samples[1::4]
samplesB = samples[2::4] + 1j * samples[3::4]

xcorr_result = np.abs(signal.correlate(samplesA, samplesB))
offset = np.argmax(xcorr_result) - N_SAMPLES
print(f"Offset: {offset}")

plt.plot(xcorr_result)
plt.show()
