import numpy as np
import socket

HOST = "127.0.0.1"
PORT = 8080
n_iq_samples = 4096
buffer_size = n_iq_samples * 2 * 2 # 2 channels, two bytes per IQ sample

while True:

    # Connect to server

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((HOST, PORT))

    # Receive IQ samples

    received_bytes = client_socket.recv(buffer_size)
    received_bytes = np.array(list(received_bytes))

    # Decode IQ samples into Numpy arrays

    ref_channel = received_bytes[0::4] + 1j * received_bytes[1::4]
    obs_channel = received_bytes[2::4] + 1j * received_bytes[3::4]

    ref_channel = (ref_channel - 127.5 * (1 + 1j)) / 127.5 # normalise data to between -1 and 1
    obs_channel = (obs_channel - 127.5 * (1 + 1j)) / 127.5 # normalise data to between -1 and 1

    print(ref_channel)

    # Do passive radar

    # Update the output plot
    img_data = np.zeros(())