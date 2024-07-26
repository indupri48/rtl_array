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

def xambg(ref_channel_samples, obs_channel_samples, sample_rate, n_range_bins=64, n_doppler_bins=64):

    """ This function computes the cross-ambiguity function of samples from the reference and observation channels """
    
    result = np.zeros((n_range_bins + 1, n_doppler_bins), dtype=np.complex64)

    # calculate the coherent processing interval in seconds
    CPI = len(ref_channel_samples) / sample_rate

    # loop over frequency bins
    for i in range(n_doppler_bins):
        # get Doppler shift for the current bin
        df = (i - 0.5 * n_doppler_bins) / CPI
        # create a frequency shifted copy of the reference signal
        ref_shifted = frequency_shift(ref_channel_samples, df, sample_rate)
        # correlate surveillance and shifted reference signals
        result[:, i] = xcorr(ref_shifted, obs_channel_samples, 0, n_range_bins)
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

# Doppler parameters
CPI = OUTPUT_BUFFER_SIZE / SAMPLE_RATE_HZ
doppler_resolution = 1 / CPI
print(f"Doppler Resolution: {doppler_resolution}Hz")
N_DOPPLER_BINS = 16
MIN_DOPPLER = -doppler_resolution * N_DOPPLER_BINS / 2
MAX_DOPPLER = doppler_resolution * N_DOPPLER_BINS / 2

# Range parameters
C = 300e3 # speed of light, km/sec
range_resolution = C / SAMPLE_RATE_HZ
N_RANGE_BINS = 128
MAX_RANGE = range_resolution * N_RANGE_BINS

# Create the radio object and begin streaming from the two channels
radio = Radio(CENTER_FREQUENCY_HZ, SAMPLE_RATE_HZ)
radio.begin_streaming()

print("Synchronising channels...")
radio.sync_channels()
print("Done!")

ref_samples, obs_samples = radio.read_samples(OUTPUT_BUFFER_SIZE)

obs_samples = np.roll(obs_samples, 0)

xambg_result = xambg(ref_samples, obs_samples, SAMPLE_RATE_HZ, n_range_bins=N_RANGE_BINS, n_doppler_bins=N_DOPPLER_BINS)

plt.xlabel("Dopper Shift (Hz)")
plt.ylabel("Range (km)")
xambg_result = np.flip(xambg_result)
plt.imshow(np.abs(xambg_result), extent=(MIN_DOPPLER, MAX_DOPPLER, 0, MAX_RANGE), aspect='auto', cmap="gray", interpolation="none")
print(np.max(np.abs(xambg_result)))
plt.savefig("passive_radar.png")

print("saved")


# Stop the radio stream
radio.stop_streaming()