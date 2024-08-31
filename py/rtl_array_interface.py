import socket
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
import multiprocessing
import threading
import queue

class ByteBuffer:
    
    """
    A FIFO queue where any number of bytes can be put into at once
    and equally any number of bytes can be taken from at once 
    
    Useful for buffering when socket recv() returns less than IQ
    frame of bytes
    """
    
    def __init__(self, buffer_size):
        
        self.write_index = 0;
        self.buffer_size = buffer_size
        self.buffer = np.zeros(buffer_size, dtype=np.uint8)
            
    def put(self, byte_list):

        n_bytes_to_write = len(byte_list)
        if self.write_index + n_bytes_to_write > self.buffer_size:
            print("ByteBuffer overflow")
            return
        
        self.buffer[self.write_index : self.write_index + n_bytes_to_write] = byte_list
        self.write_index += n_bytes_to_write

    def get(self, n_bytes_to_read):
        
        if n_bytes_to_read > self.buffer_size:
            print("Number of bytes requested exceeds ByteBuffer size")
        
        byte_list = self.buffer[0 : n_bytes_to_read]
        self.buffer = np.roll(self.buffer, -n_bytes_to_read)
        self.write_index -= n_bytes_to_read
        if self.write_index < 0:
            self.write_index = 0
        
        return byte_list
    
    def get_size(self):
        
        return self.write_index

class IQFrameBuffer:
    
    def __init__(self, frame_size, n_frames):
        
        self.read_counter = 0
        self.write_counter = 0;
        
        self.n_frames = n_frames
        self.frame_size = frame_size
        
        self.buffer = np.zeros(n_frames * frame_size, dtype=np.uint8)
            
    def write_frame(self, frame):

        write_counter_index = self.write_counter % self.n_frames
        
        start_index = write_counter_index * self.frame_size
        stop_index = (write_counter_index + 1) * self.frame_size
        self.buffer[start_index : stop_index] = frame
        self.write_counter += 1
        
        
    def read_frame(self):
        
        read_counter_index = self.read_counter % self.n_frames
        
        start_index = read_counter_index * self.frame_size
        stop_index = (read_counter_index + 1) * self.frame_size
        frame = self.buffer[start_index : stop_index]
        self.read_counter += 1
        
        return frame
        
def load_iq_samples_from_file(filename, n_channels):
    
    raw_data = np.fromfile(filename, dtype=np.uint8)
    print(raw_data)
    return extract_iq_data(raw_data, n_channels)

def extract_iq_data(raw_data, n_channels):
    
    # The maximum number of IQ samples that can be extraced
    n_samples_per_channel = int(len(raw_data) / (2 * n_channels))
    
    samples = np.zeros((n_channels, n_samples_per_channel), dtype=np.complex64)
    
    for channel_number in range(n_channels):
        channel_I = np.array(raw_data[channel_number * 2 + 0 :: n_channels * 2], dtype=np.float32)
        print(channel_I)
        channel_Q = np.array(raw_data[channel_number * 2 + 1:: n_channels * 2], dtype=np.float32)
        channel_I = (2 * channel_I) / 255 - 1
        channel_Q = (2 * channel_Q) / 255 - 1
        print(channel_I)
        channel_samples = channel_I + 1j * channel_Q
        samples[channel_number:] = channel_samples
        
    return samples
        
class RadioInterface:
    
    def __init__(self, radio_hostname='127.0.0.1', radio_port=45565):

        self.connected = False
        self.radio_hostname = radio_hostname
        self.radio_port = radio_port
        self.socket = None
        
        self.n_channels = 2
        self.n_channel_samples = 2 ** 17
        self.channel_size = self.n_channel_samples * 2
        self.iq_frame_size = self.channel_size * self.n_channels
        self.byte_buffer = ByteBuffer(self.iq_frame_size * 8)
        self.frame_buffer = IQFrameBuffer(self.iq_frame_size, 8)
        
    def connect(self, hostname="127.0.0.1", port=45565):

        try:
            # Create a socket and connect to the radio server
            tmp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tmp_socket.connect((hostname, port))
            self.socket = tmp_socket
            self.radio_hostname = hostname
            self.radio_port = port
            self.connected = True
            self.receiver_thread = threading.Thread(target=self.receive_iq_frames, args=())
            self.receiver_thread.start()
            print(f"Connected to radio server at {hostname}:{port}")
        except socket.error:
            print(f"Failed to connect to radio server at {hostname}:{port}")

    def disconnect(self):

        if not self.connected:
            print("Disconnect not possible. Not connected to server")
            return

        self.socket.close()
        self.connected = False

        print(f"Disconnected from radio server at {self.radio_hostname}:{self.radio_port}")

    def receive_iq_frames(self):
                        
        while self.connected:

            received_bytes = list(self.socket.recv(self.iq_frame_size))
            print(f"Received {len(received_bytes)}")
            
            self.byte_buffer.put(received_bytes)
            print(f"Byte buffer is now {self.byte_buffer.get_size()} bytes long")

            if self.byte_buffer.get_size() >= self.iq_frame_size:
                
                print("Received an IQ Frame!")
            
                iq_frame = self.byte_buffer.get(self.iq_frame_size)
                self.frame_buffer.write_frame(iq_frame)
        
# radio = RadioInterface()
# radio.connect(hostname="192.168.1.64")

# for i in range(1000):
    
#     #b = radio.frame_buffer.read_frame()[0 : 4]
#     #print(b)
#     time.sleep(0.1)

# radio.disconnect()
