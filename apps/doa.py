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

def calculate_doa(samples0, samples1):

    '''
    Calculate the signal power for a sweep of angles. Using conventional beamforming
    '''
    Nr = 2
    d = 0.5
    r = np.array([samples0, samples1])
    theta_scan = np.linspace(-1*np.pi, np.pi, 1000) # 1000 different thetas between -180 and +180 degrees
    results = []
    for theta_i in theta_scan:
        w = np.exp(-2j * np.pi * d * np.arange(Nr) * np.sin(theta_i)) # Conventional, aka delay-and-sum, beamformer
        r_weighted = w.conj().T @ r # apply our weights. remember r is 3x10000
        results = np.append(results, 10*np.log10(np.var(r_weighted))) # power in signal, in dB so its easier to see small and large lobes at the same time
        results -= np.max(results) # normalize
    print(results)
    # print angle that gave us the max value
    print(theta_scan[np.argmax(results)] * 180 / np.pi) # 19.99999999999998

    return theta_scan, results

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 8080  # The port used by the server

n_samples = 4096
plt.ion()

plt.figure(1)
ax = plt.subplot(1, 1, 1, polar=True)
#p, = ax.plot([0, 0], [0, 1])
plt.show(block = False)
plt.ion()

while True:

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                                
        s.connect((HOST, PORT))
        raw_bytes = s.recv(n_samples * 4)
        s.close()

        samples0, samples1 = read_samples(raw_bytes)

        theta, powers = calculate_doa(samples0, samples1)

        powers = np.abs(powers)
        plt.clf()
        #plt.plot(theta, powers)
        plt.polar(theta, powers)
        plt.draw()

        plt.pause(0.05)

#        print("loop")
