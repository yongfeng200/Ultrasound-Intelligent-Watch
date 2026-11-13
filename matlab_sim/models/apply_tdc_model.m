function measurement = apply_tdc_model(tof_down_s, tof_up_s, cfg)
%APPLY_TDC_MODEL Add offsets, jitter, quantization, and status flags.

n = numel(tof_down_s);
measurement.tof_down_ideal_s = tof_down_s(:);
measurement.tof_up_ideal_s = tof_up_s(:);
measurement.tof_down_raw_s = tof_down_s(:);
measurement.tof_up_raw_s = tof_up_s(:);
measurement.tof_down_ticks = zeros(n, 1);
measurement.tof_up_ticks = zeros(n, 1);
measurement.valid_down = true(n, 1);
measurement.valid_up = true(n, 1);
measurement.status_down = repmat("VALID", n, 1);
measurement.status_up = repmat("VALID", n, 1);

noise_down = cfg.tdc.jitter_std_s .* randn(n, 1);
noise_up = cfg.tdc.jitter_std_s .* randn(n, 1);
measurement.tof_down_raw_s = (tof_down_s(:) + cfg.tdc.offset_down_s + noise_down) ...
    .* cfg.tdc.gain_down;
measurement.tof_up_raw_s = (tof_up_s(:) + cfg.tdc.offset_up_s + noise_up) ...
    .* cfg.tdc.gain_up;

measurement.tof_down_ticks = round(measurement.tof_down_raw_s ./ cfg.tdc.tick_period_s);
measurement.tof_up_ticks = round(measurement.tof_up_raw_s ./ cfg.tdc.tick_period_s);
measurement.tof_down_raw_s = measurement.tof_down_ticks .* cfg.tdc.tick_period_s;
measurement.tof_up_raw_s = measurement.tof_up_ticks .* cfg.tdc.tick_period_s;

% Inject independent timeout and outlier states.
timeout_down = rand(n, 1) < cfg.tdc.timeout_probability;
timeout_up = rand(n, 1) < cfg.tdc.timeout_probability;
outlier_down = rand(n, 1) < cfg.tdc.outlier_probability;
outlier_up = rand(n, 1) < cfg.tdc.outlier_probability;

measurement.tof_down_raw_s(outlier_down) = measurement.tof_down_raw_s(outlier_down) + ...
    cfg.tdc.outlier_size_s;
measurement.tof_up_raw_s(outlier_up) = measurement.tof_up_raw_s(outlier_up) - ...
    cfg.tdc.outlier_size_s;
measurement.tof_down_ticks(outlier_down) = round(measurement.tof_down_raw_s(outlier_down) ./ cfg.tdc.tick_period_s);
measurement.tof_up_ticks(outlier_up) = round(measurement.tof_up_raw_s(outlier_up) ./ cfg.tdc.tick_period_s);

measurement.valid_down(timeout_down) = false;
measurement.valid_up(timeout_up) = false;
measurement.status_down(timeout_down) = "TIMEOUT";
measurement.status_up(timeout_up) = "TIMEOUT";
measurement.status_down(outlier_down & ~timeout_down) = "OUTLIER";
measurement.status_up(outlier_up & ~timeout_up) = "OUTLIER";
end
