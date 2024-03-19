import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt

N_SAMPLES = 1024 * 40
samples = np.fromfile("samples.dat", dtype=np.int8, count=N_SAMPLES * 2 * 2)
samplesA = samples[0::4] + 1j * samples[1::4]
samplesB = samples[2::4] + 1j * samples[3::4]

for i in range(40):
    n_zeros = np.count_nonzero(samples[i * 4096:(i+1)*4096]==0)
    print(i, n_zeros)

plt.plot(samplesA.real)
plt.show()
