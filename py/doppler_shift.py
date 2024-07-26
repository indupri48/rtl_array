import time
import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
from radio import Radio
from datetime import datetime

def frequency_shift(samples, delta_frequency, sample_rate, phase_offset=0):

    nn = np.arange(len(samples), dtype=np.complex64)
    
    return samples * np.exp(1j * 2 * np.pi * delta_frequency * nn / sample_rate + 1j * phase_offset)

def xcorr(s1, s2, nlead, nlag):
    ''' cross-correlate s1 and s2 with sample offsets from -1*nlag to nlead'''
    return signal.correlate(s1, np.pad(s2, (nlag, nlead), mode='constant'),
        mode='valid')


def declutter(channel_samples):

    """ This function attempts to remove cluttered signals from the observation channel and lower the noise floor """

    return channel_samples

def xambg(ref_channel_samples, obs_channel_samples, sample_rate, n_range_bins=4, n_doppler_bins=4):

    """ This function computes the cross-ambiguity function of samples from the reference and observation channels """
    
    result = np.zeros((n_range_bins + 1, n_doppler_bins))

    # calculate the coherent processing interval in seconds
    CPI = len(ref_channel_samples) / sample_rate

    # loop over frequency bins
    for i in range(n_doppler_bins):
        # get Doppler shift for the current bin
        df = (i - 0.5 * n_doppler_bins) / CPI
        # create a frequency shifted copy of the reference signal
        ref_shifted = frequency_shift(ref_channel_samples, df, sample_rate)
        # correlate surveillance and shifted reference signals
        result[i] = xcorr(ref_shifted, obs_channel_samples, n_range_bins, 0)
        print(i)
    return result

def doppler(ref_channel_samples, obs_channel_samples, sample_rate, doppler_range):
    
    decimation_factor = int(sample_rate // doppler_range)

    ref_samples = signal.decimate(ref_channel_samples, decimation_factor)
    obs_samples = signal.decimate(obs_channel_samples, decimation_factor)

    conj_product = ref_samples * np.conj(obs_samples)
    fft_result = np.fft.fftshift(np.fft.fft(conj_product))
    fft_result = np.abs(fft_result) ** 2

    return fft_result

CENTER_FREQUENCY_HZ = 103.7e6 # 103.7MHz
SAMPLE_RATE_HZ = 2.4e6 # 2.4MHz

OUTPUT_BUFFER_SIZE = 4096 * 128 # ~250ms
cpi_interval = OUTPUT_BUFFER_SIZE / SAMPLE_RATE_HZ
doppler_resolution = 1.0 / cpi_interval
n_doppler_bins = 256
doppler_range = int(n_doppler_bins * doppler_resolution)
print(f"Doppler range: {doppler_range}Hz")

# Create the radio object and begin streaming from the two channels
radio = Radio(CENTER_FREQUENCY_HZ, SAMPLE_RATE_HZ)
radio.begin_streaming()

# input("Press enter to synchronise the 2 radio channels")
radio.sync_channels()

n_rows = 64 * 4
data = np.zeros((n_rows, n_doppler_bins))

start_time = time.time()

# Main loop of operation
for i in range(n_rows):
    
    ref_samples, obs_samples = radio.read_samples(OUTPUT_BUFFER_SIZE)

    data[i] = doppler(ref_samples, obs_samples, SAMPLE_RATE_HZ, doppler_range=doppler_range)

    #data[i] = (data[i] - min(data[i])) / (max(data[i] - min(data[i])))
    #data[i] = 10.0 * np.log10(data[i])
    print(f"{i + 1} / {n_rows}")

    #time.sleep(0.25 / 4)
    
end_time = time.time()

plt.xlabel("Doppler Shift (Hz)")
plt.ylabel("Time")
plt.imshow(data, extent=(-doppler_range / 2, doppler_range / 2, end_time - start_time, 0), aspect="auto", interpolation="none")
plt.savefig("doppler_shift.png")

print("saved")

# Stop the radio stream
radio.stop_streaming()