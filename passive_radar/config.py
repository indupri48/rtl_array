def load_configuration():

    config = {}

    # constants
    config['c'] = 300e3 # speed of light in a vacuum in kms^-1

    # system configuration
    config['sample_rate_hz'] = 2400000 # 2.4MHz

    # Range

    config['range_bin_width_km'] = config['c'] / config['range_bin_width']
    config['max_range_km'] = 50 # km. User defined
    config['n_range_bins'] = config['max_range_km'] / config['range_bin_width']

    # Doppler

    config['doppler_bin_width_hz'] = 1.0 / config[]

    # range bins
