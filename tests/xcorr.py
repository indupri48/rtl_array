import socket
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import scipy.signal as signal
import time

def read_samples(raw_bytes):
    
    raw_bytes = list(raw_bytes)
    raw_data = np.array(raw_bytes)
    raw_data = (raw_data - 127.5) / 127.5

    I0 = raw_data[0::4] # dongle 0 I data
    Q0 = raw_data[1::4] # dongle 0 Q data
    I1 = raw_data[2::4] # dongle 1 I data
    Q1 = raw_data[3::4] # dongle 0 Q data

    samples0 = I0 + 1j * Q0
    samples1 = I1 + 1j * Q1

    return samples0, samples1

def xcorr(samples0, samples1):

    xcorr_result = signal.correlate(samples0, samples1)
    return np.abs(xcorr_result)

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 8080  # The port used by the server

n_samples = 4096
plt.ion()

plt.figure(1)
ax = plt.subplot(2,1,1)
p, = ax.plot([0, 1], [0, 1])
plt.show(block = False)
plt.ion()

while True:

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                                
        s.connect((HOST, PORT))
        raw_bytes = s.recv(n_samples * 4)
        s.close()

        samples0, samples1 = read_samples(raw_bytes)

        c = xcorr(samples0, samples1)
        max_i = np.argmax(c)
        sideband_points = 5
        peak_y = c[max_i - sideband_points : max_i + sideband_points + 1]
        peak_x = np.arange(-sideband_points, sideband_points + 1)

        p = np.polyfit(peak_x, peak_y, 2)
        center_i = p[1] / (2 * p[0])
        print(max_i, center_i)

        plt.clf()
        plt.plot(c)
        
        plt.ylim(0, 400)
        plt.draw()

        plt.pause(0.05)

#        print("loop")
