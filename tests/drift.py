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

def animate(i):
    
    global x
    global y

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                                
        s.connect((HOST, PORT))
        raw_bytes = s.recv(n_samples * 4)
        s.close()

        samples0, samples1 = read_samples(raw_bytes)

        c = xcorr(samples0, samples1)
        max_i = np.argmax(c)
        print(max_i)

        sideband_points = 5
        peak_y = c[max_i - sideband_points : max_i + sideband_points + 1]
        peak_x = np.arange(-sideband_points, sideband_points + 1)

        p = np.polyfit(peak_x, peak_y, 2)
        center_i = p[1] / (2 * p[0])
        print(max_i, center_i)

        y = np.append(y, center_i)
        x = np.append(x, len(x))

        plt.cla()
        plt.plot(x,y)
        plt.xlabel('Time (s)')
        plt.ylabel('Cross-correlation peak')
        #plt.ylim(-n_samples, n_samples)
        plt.tight_layout()


x = np.array([])
y = np.array([])

ani = animation.FuncAnimation(plt.gcf(), animate, interval=250)
plt.show()