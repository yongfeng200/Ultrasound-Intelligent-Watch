function cfg = default_config()
%DEFAULT_CONFIG Configuration for the LS1D ultrasonic water-meter simulation.
% All internal calculations use SI units.

cfg.sim.duration_s = 30;
cfg.sim.sample_period_s = 0.25;
cfg.sim.random_seed = 20260929;
cfg.sim.plot_results = true;

% Example geometry. Replace with measured meter parameters.
cfg.physical.pipe_diameter_m = 0.020;
cfg.physical.sound_path_m = 0.100;
cfg.physical.angle_deg = 45;
cfg.physical.reference_temperature_c = 20;
cfg.physical.reference_sound_speed_m_s = 1482;
cfg.physical.sound_speed_slope_m_s_per_c = 2.5;
cfg.physical.flow_profile_factor = 1.0;

% TDC settings are explicit model parameters, not a claim about the
% undocumented internal fixed-point scale used by the target hardware.
cfg.tdc.tick_period_s = 1e-9;
cfg.tdc.offset_down_s = 20e-9;
cfg.tdc.offset_up_s = -10e-9;
cfg.tdc.gain_down = 1.0;
cfg.tdc.gain_up = 1.0;
cfg.tdc.timeout_probability = 0.002;
cfg.tdc.outlier_probability = 0.01;
cfg.tdc.outlier_size_s = 0.5e-6;
cfg.tdc.jitter_std_s = 2e-9;

cfg.noise.temperature_std_c = 0.05;
cfg.noise.random_seed = cfg.sim.random_seed;

cfg.filter.method = "median";
cfg.filter.window_length = 5;
cfg.filter.iir_alpha = 0.20;
cfg.filter.hampel_threshold = 3.0;
cfg.filter.max_velocity_step_m_s = 3.0;

cfg.integration.invalid_policy = "skip";

cfg.csv.timestamp_column = "timestamp_s";
cfg.csv.tick_columns = ["tof_down_ticks", "tof_up_ticks"];
cfg.csv.tof_columns = ["tof_down_s", "tof_up_s"];

cfg.output.directory = fullfile(fileparts(mfilename("fullpath")), "..", "results");
end
