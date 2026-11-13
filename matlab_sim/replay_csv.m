function results = replay_csv(file_path, varargin)
%REPLAY_CSV Replay a normalized LS1D measurement CSV through the algorithm.
root = fileparts(mfilename("fullpath"));
addpath(genpath(root));
cfg = default_config();
if nargin >= 2 && isstruct(varargin{1})
    cfg = varargin{1};
end

data = load_measurement_csv(file_path, cfg);
[velocity_raw_m_s, velocity_valid] = time_to_velocity(...
    data.tof_down_s, data.tof_up_s, cfg, "exact");
velocity_valid = velocity_valid & data.valid_down & data.valid_up;
[velocity_filtered_m_s, filter_valid, reject_reason] = ...
    filter_measurement(velocity_raw_m_s, velocity_valid, cfg);
flow_filtered_m3_s = velocity_to_flow(velocity_filtered_m_s, cfg);
volume_m3 = integrate_flow(data.timestamp_s, flow_filtered_m3_s, filter_valid, cfg);

results = struct();
results.timestamp_s = data.timestamp_s;
results.velocity_raw_m_s = velocity_raw_m_s;
results.velocity_filtered_m_s = velocity_filtered_m_s;
results.flow_filtered_m3_s = flow_filtered_m3_s;
results.flow_filtered_l_min = flow_filtered_m3_s * 60000;
results.volume_m3 = volume_m3;
results.volume_l = volume_m3 * 1000;
results.valid = filter_valid;
results.reject_reason = reject_reason;
results.metrics = struct();

reference = data.flow_reference_lpm / 60000;
if any(isfinite(reference))
    results.metrics = calculate_metrics(flow_filtered_m3_s, reference, filter_valid & isfinite(reference));
end
end
