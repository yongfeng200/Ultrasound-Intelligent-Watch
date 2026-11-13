function run_regression_tests()
%RUN_REGRESSION_TESTS Execute lightweight MATLAB regression assertions.
root = fileparts(mfilename("fullpath"));
addpath(genpath(root));
cfg = default_config();
cfg.sim.plot_results = false;
cfg.tdc.jitter_std_s = 0;
cfg.tdc.offset_down_s = 0;
cfg.tdc.offset_up_s = 0;
cfg.tdc.timeout_probability = 0;
cfg.tdc.outlier_probability = 0;

% Physical direction, zero flow, and inversion checks.
v = [-0.4; 0; 0.4];
temperature = cfg.physical.reference_temperature_c * ones(size(v));
[td, tu] = simulate_ultrasonic_times(v, temperature, cfg);
assert(abs(td(2) - tu(2)) < 1e-15, "Zero-flow TOFs should match.");
assert(td(3) < tu(3) && td(1) > tu(1), "TOF direction sign is wrong.");
[v_recovered, valid] = time_to_velocity(td, tu, cfg, "exact");
assert(all(valid) && max(abs(v_recovered - v)) < 1e-10, "Exact velocity inversion failed.");

% Quantization bound.
quantized = apply_tdc_model(td, tu, cfg);
assert(max(abs(quantized.tof_down_raw_s - td)) <= cfg.tdc.tick_period_s / 2 + eps, ...
    "TDC quantization exceeds half a tick.");

% Flow integration with a known constant.
t = [0; 1; 3];
q = 2e-6 * ones(size(t));
volume = integrate_flow(t, q, true(size(t)), cfg);
assert(abs(volume(end) - q(1) * (t(end) - t(1))) < 1e-12, "Flow integration failed.");

% Robust filtering rejects an injected isolated impulse.
values = ones(21, 1);
values(11) = 100;
[~, accepted] = filter_measurement(values, true(size(values)), cfg);
assert(~accepted(11), "Filter failed to reject isolated impulse.");

% Example CSV replay smoke test.
replay = replay_csv(fullfile(root, "data", "sample_measurement.csv"), cfg);
assert(numel(replay.timestamp_s) == 10, "CSV replay returned unexpected sample count.");

fprintf("All MATLAB regression tests passed.\n");
end
