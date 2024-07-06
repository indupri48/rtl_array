from radio import Radio
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import time

radio = Radio(103.7e6, 2.4e6)
radio.begin_streaming()

n_samples = 512
n_rows = 64
data = np.zeros((n_rows * 2, n_samples * 2 - 1))

offsets = []

for i in range(n_rows):

    samples0, samples1 = radio.read_samples(n_samples)
    xcorr_result = signal.correlate(samples0, samples1)
    xcorr_result = np.abs(xcorr_result)

    offset = np.argmax(xcorr_result) - len(samples0)
    offsets = offsets + [offset]
    #print(offset, np.max(xcorr_result))

    data[i] = xcorr_result

    print(f"Recorded {i+1} / {n_rows}. Channel offset: {offset}")
      
    time.sleep(0.1)

print("Synchronising channels...")
sync_success = radio.sync_channels()

if sync_success:
    print(f"Channels synchronised! SDR1 delay: {radio.sdr1_delay}, SDR2 delay: {radio.sdr2_delay}")
else:
    print("Failed to synchronise channels -- poor input signal")

for i in range(n_rows):

    samples0, samples1 = radio.read_samples(n_samples)
    xcorr_result = signal.correlate(samples0, samples1)
    xcorr_result = np.abs(xcorr_result)
    offset = np.argmax(xcorr_result) - len(samples0)
    offsets = offsets + [offset]
    #print(offset, np.max(xcorr_result))

    data[i + n_rows] = xcorr_result

    print(f"Recorded {i+1} / {n_rows}. Channel offset: {offset}")
   
    time.sleep(0.1)

radio.stop_streaming()

plt.title("Cross correlation of two RTL-SDR streams with shared 28.8MHz clocks")
plt.xlabel("Cross correlation index")
plt.ylabel("Cross correlation magnitude")
plt.imshow(data, aspect="auto", interpolation='none')
plt.savefig("radio_test.png")
plt.show()