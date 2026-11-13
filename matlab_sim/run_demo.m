function results = run_demo(varargin)
%RUN_DEMO Run the LS1D ultrasonic water-meter algorithm simulation.
% Example: results = run_demo();

root = fileparts(mfilename("fullpath"));
addpath(genpath(root));
cfg = default_config();
if nargin >= 1 && isstruct(varargin{1})
    cfg = varargin{1};
end
rng(cfg.sim.random_seed);

[timestamp_s, velocity_true_m_s, temperature_c] = generate_flow_profile(cfg);
[tof_down_ideal_s, tof_up_ideal_s, sound_speed_m_s] = ...
    simulate_ultrasonic_times(velocity_true_m_s, temperature_c, cfg);
measurement = apply_tdc_model(tof_down_ideal_s, tof_up_ideal_s, cfg);

raw_valid = measurement.valid_down & measurement.valid_up;
[velocity_raw_m_s, velocity_valid] = time_to_velocity(...
    measurement.tof_down_raw_s, measurement.tof_up_raw_s, cfg, "exact");
velocity_valid = velocity_valid & raw_valid;

[velocity_filtered_m_s, filter_valid, reject_reason] = ...
    filter_measurement(velocity_raw_m_s, velocity_valid, cfg);
flow_true_m3_s = velocity_to_flow(velocity_true_m_s, cfg);
flow_raw_m3_s = velocity_to_flow(velocity_raw_m_s, cfg);
flow_filtered_m3_s = velocity_to_flow(velocity_filtered_m_s, cfg);
volume_m3 = integrate_flow(timestamp_s, flow_filtered_m3_s, filter_valid, cfg);

results = struct();
results.timestamp_s = timestamp_s;
results.temperature_c = temperature_c;
results.sound_speed_m_s = sound_speed_m_s;
results.velocity_true_m_s = velocity_true_m_s;
results.velocity_raw_m_s = velocity_raw_m_s;
results.velocity_filtered_m_s = velocity_filtered_m_s;
results.flow_true_m3_s = flow_true_m3_s;
results.flow_raw_m3_s = flow_raw_m3_s;
results.flow_filtered_m3_s = flow_filtered_m3_s;
results.flow_true_l_min = flow_true_m3_s * 60000;
results.flow_filtered_l_min = flow_filtered_m3_s * 60000;
results.volume_m3 = volume_m3;
results.volume_l = volume_m3 * 1000;
results.tof_down_s = measurement.tof_down_raw_s;
results.tof_up_s = measurement.tof_up_raw_s;
results.tof_down_us = measurement.tof_down_raw_s * 1e6;
results.tof_up_us = measurement.tof_up_raw_s * 1e6;
results.tof_down_ticks = measurement.tof_down_ticks;
results.tof_up_ticks = measurement.tof_up_ticks;
results.valid = filter_valid;
results.reject_reason = reject_reason;
results.metrics = calculate_metrics(velocity_filtered_m_s, velocity_true_m_s, filter_valid);

if cfg.sim.plot_results
    plot_measurement_results(results);
end

if ~exist(cfg.output.directory, "dir")
    mkdir(cfg.output.directory);
end
writetable(struct2table(results_for_table(results)), ...
    fullfile(cfg.output.directory, "simulation_samples.csv"));
write_summary(results.metrics, cfg.output.directory);
end

function output = results_for_table(results)
output = struct();
fields = {"timestamp_s", "velocity_true_m_s", "velocity_raw_m_s", ...
    "velocity_filtered_m_s", "flow_true_l_min", "flow_filtered_l_min", ...
    "volume_l", "tof_down_us", "tof_up_us", "valid"};
for k = 1:numel(fields)
    output.(fields{k}) = results.(fields{k});
end
output.reject_reason = results.reject_reason;
end

function write_summary(metrics, directory)
file_id = fopen(fullfile(directory, "simulation_summary.txt"), "w");
cleanup = onCleanup(@() fclose(file_id));
fprintf(file_id, "LS1D ultrasonic water-meter simulation summary\n");
fprintf(file_id, "Generated: %s\n", datestr(now, 31));
fprintf(file_id, "Samples: %d\n", metrics.sample_count);
fprintf(file_id, "Valid ratio: %.6f\n", metrics.valid_ratio);
fprintf(file_id, "Velocity MAE (m/s): %.9g\n", metrics.mae);
fprintf(file_id, "Velocity RMSE (m/s): %.9g\n", metrics.rmse);
fprintf(file_id, "Velocity bias (m/s): %.9g\n", metrics.bias);
end
