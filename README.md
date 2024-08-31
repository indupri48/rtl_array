# rtl_array

## Description
**[Note that *rtl_array* is currently a work-in-progress project]**

*rtl_array* is designed to stream IQ data from multiple RTL-SDRs together. It is similar to *rtl_sdr* and *rtl_tcp* from librtlsdr but it can support multiple dongles and so appears as a multi-channel SDR. The output stream can be stdout, a file, or a TCP connection (with *rtl_array* acting as the server).

This project is influenced by the KrakenSDR Acquistion software, Heimdall. It is not aimed to be a replacement but instead a more lightweight alternative and also a learning exercise for me!
 
## Building and Running
The building and running of this software has been kept as simple as possible. The method below has only been tested on my Raspberry Pi 5 so far.

*rtl_array* depends on librtlsdr. Once librtlsdr has been installed, run
```
bash build.sh
```
to create a binary called *rtl_array*. Run the binary using
```
./rtl_array
```

## Applications

For an application where the dongles can be treated independently, *rtl_array* would be excessive. A better solution would be to have multiple instances of e.g. *rtl_tcp* instead. *rtl_array* is useful where the channels are designed to be used together.

A common pattern in SDR software is to separate the IQ data acquisition software and the signal processign software. This allows either piece of software to be substituted with minimal fuss.
Therefore, the role of *rtl_array* is to group multiple RTL-SDR dongles together to create a low-cost but fiddly multichannel receiver. The hope is to take as much fiddliness away from the user as possible. 

### Stream Format

The data stream has the same repeating pattern where the IQ byte pair from each dongle is sent in turn, creating an interleaved stream of IQ samples.

## TODO List
* Add channel corrections (see below). Currently, *rtl_array* streams the samples 'as-is'.
* Add command line arguments
* Add suite of tests to test system performance. These would test coherency, signal levels etc.
* Organise the code better

## Channel Corrections
The hardware configurtaion in mind for rtl_array are several RTL-SDR dongles connected to the same computer, forming a multi-channel receiver. With no further effort, these dongles with not be calibrated with respect to one another. This is okay for applications where the dongles are effectively independent from one another e.g. spectrum monitoring.

However, applications that intend to combine the data from the channels require some level of calibration. There are several calibrations that may need to be performed.
During calibration, a common reference signal needs to be provided to each channel. The receiver needs to be calibrated every time it is retuned to a different frequency so a noise source is commonly used for a reference because of its wideband nature.

### Bulk Delay
Each dongle will begin streaming IQ samples at different times which results in a time delay of possibly several hundred samples difference. This difference is called the bulk delay. This software attempts to start the streams together but there will inevitably be some delay between dongles due to the OS.

To calculate the difference between channels, cross-correlation is used. The index of the peak is the bulk delay.

To correct bulk delay, there is an index offset for each dongle's sample buffer to permanent get samples added later in the buffer. This is artificial delay because nothing about the incoming samples being streamed from the dongles is changed. However, to an application receiving samples from rtl_array, there's no way of telling.

### Sample Drift
This is only a problem for dongles that do not share a common 28.8MHz clock. 

### Frequency Drift
TODO 

### Phase coherency
