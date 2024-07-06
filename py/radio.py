import threading
import time
import multiprocessing

import numpy as np
import scipy.signal as signal
from rtlsdr import RtlSdr

class Radio:

    def __init__(self, center_frequency_Hz, sample_rate_Hz):

        self.center_frequency_Hz = center_frequency_Hz
        self.sample_rate_Hz = sample_rate_Hz

        # Set up RTL-SDR objects

        self.sdr1 = RtlSdr(device_index=0)
        self.sdr1.center_freq = center_frequency_Hz
        self.sdr1.sample_rate = sample_rate_Hz
        self.sdr1.gain = 40

        self.sdr2 = RtlSdr(device_index=1)
        self.sdr2.center_freq = center_frequency_Hz
        self.sdr2.sample_rate = sample_rate_Hz
        self.sdr2.gain = 40

        # Set up threading and buffers
        
        self.chunk_size = 4096
        self.n_chunks_in_buffer = 512
        self.buffer_size = self.chunk_size * self.n_chunks_in_buffer
        self.sdr1_buffer = np.zeros(self.buffer_size, dtype=np.complex64)
        self.sdr2_buffer = np.zeros(self.buffer_size, dtype=np.complex64)
        # self.sdr1_queue = multiprocessing.Queue()
        # self.sdr2_queue = multiprocessing.Queue()
        self.sdr1_write_counter = 0
        self.sdr2_write_counter = 0
        self.sdr1_producer_thread = threading.Thread(target=self.sdr1_producer_process, name="SDR1 Producer Thread", args=[])
        self.sdr2_producer_thread = threading.Thread(target=self.sdr2_producer_process, name="SDR2 Producer Thread", args=[])
        # self.sdr1_writer_thread = threading.Thread(target=self.sdr1_writer_process, name="SDR1 Writer Thread", args=[])
        # self.sdr2_writer_thread = threading.Thread(target=self.sdr2_writer_process, name="SDR2 Writer Thread", args=[])
        self.reading = False
        self.sdr1_writing = False
        self.sdr2_writing = False

        # Channel delays
        self.sdr1_delay = 0
        self.sdr2_delay = 0

        # Event
        self.event = multiprocessing.Event()

        print("Created radio object succesfully")

    def sync_channels(self, n_samples=4096, n_captures=8):

        offsets = np.zeros(n_captures)

        for capture_index in range(n_captures):

            samples0, samples1 = self.read_samples(n_samples)

            xcorr_result = signal.correlate(samples0, samples1)
            xcorr_result = np.abs(xcorr_result)
            offset = np.argmax(xcorr_result) - len(samples0)
            offsets[capture_index] = offset
            time.sleep(0.1)
        
        channel_offset = int(np.median(offsets))

        n_different = np.count_nonzero(offsets - channel_offset)
        
        if n_different > 2:
            return False

        if channel_offset < 0:
            self.sdr1_delay = 0
            self.sdr2_delay = -channel_offset
        else:
            self.sdr1_delay = channel_offset
            self.sdr2_delay = 0

        return True

    def read_samples(self, n_samples): 
        
        if n_samples > self.buffer_size - max(self.sdr1_delay, self.sdr2_delay):
            raise ValueError(f"Buffer size of {self.buffer_size} samples is too small to read {n_samples} samples!")

        while (self.sdr1_writing or self.sdr2_writing) or self.sdr1_write_counter != self.sdr2_write_counter:
            time.sleep(0.000000001)
        self.reading = True
        sdr1_start_index = ((self.sdr1_write_counter-1) % self.n_chunks_in_buffer) * self.chunk_size + self.sdr1_delay
        sdr2_start_index = ((self.sdr2_write_counter-1) % self.n_chunks_in_buffer) * self.chunk_size + self.sdr2_delay

        sdr1_overspill = max(0, sdr1_start_index + n_samples - self.buffer_size)
        sdr2_overspill = max(0, sdr2_start_index + n_samples - self.buffer_size)
        #print(sdr1_start_index, sdr2_start_index)
        if sdr1_overspill > 0:
            sdr1_output_buffer = np.append(self.sdr1_buffer[sdr1_start_index : sdr1_start_index + n_samples - sdr1_overspill], self.sdr1_buffer[: sdr1_overspill])
        else:
            sdr1_output_buffer = self.sdr1_buffer[sdr1_start_index : sdr1_start_index + n_samples]

        if sdr2_overspill > 0:
            sdr2_output_buffer = np.append(self.sdr2_buffer[sdr2_start_index : sdr2_start_index + n_samples - sdr2_overspill], self.sdr2_buffer[: sdr2_overspill])
        else:
            sdr2_output_buffer = self.sdr2_buffer[sdr2_start_index : sdr2_start_index + n_samples]

        self.reading = False

        return sdr1_output_buffer, sdr2_output_buffer

    def begin_streaming(self):

        self.sdr1_write_counter = 0
        self.sdr2_write_counter = 0

        self.sdr1_producer_thread.start()
        self.sdr2_producer_thread.start()

        time.sleep(1)

        self.event.set()

        self.event.set()

    def stop_streaming(self):

        # stop the sdr processes
        self.sdr1.cancel_read_async()
        self.sdr2.cancel_read_async()

    def sdr1_producer_process(self):
        
        self.event.wait()

        self.sdr1.read_samples_async(callback=self.sdr1_callback, num_samples=self.chunk_size, context=None)

    def sdr2_producer_process(self):

        self.event.wait()

        self.event.wait()
        self.sdr2.read_samples_async(callback=self.sdr2_callback, num_samples=self.chunk_size, context=None)

    # def sdr1_writer_process(self):

    # def sdr2_writer_process(self):
        

    def sdr1_callback(self, sample_chunk, context):

        # self.sdr1_queue.put(sample_chunk)
        
        while self.reading:
            time.sleep(0.000000001)
        self.sdr1_writing = True
        
        sdr1_start_index = (self.sdr1_write_counter % self.n_chunks_in_buffer) * self.chunk_size
        self.sdr1_buffer[sdr1_start_index : sdr1_start_index + self.chunk_size] = sample_chunk
        self.sdr1_write_counter += 1

        self.sdr1_writing = False

    def sdr2_callback(self, sample_chunk, context):
        
        # self.sdr1_queue.put(sample_chunk)
        
        while self.reading:
            time.sleep(0.000000001)
        self.sdr2_writing = True
        
        sdr2_start_index = (self.sdr2_write_counter % self.n_chunks_in_buffer) * self.chunk_size
        self.sdr2_buffer[sdr2_start_index : sdr2_start_index + self.chunk_size] = sample_chunk
        self.sdr2_write_counter += 1

        self.sdr2_writing = False