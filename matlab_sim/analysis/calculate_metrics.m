function metrics = calculate_metrics(estimate, reference, valid)
%CALCULATE_METRICS Calculate basic error metrics for valid samples.
valid = logical(valid(:)) & isfinite(estimate(:)) & isfinite(reference(:));
error_value = estimate(valid) - reference(valid);
metrics.sample_count = numel(estimate);
metrics.valid_count = nnz(valid);
metrics.valid_ratio = metrics.valid_count / max(metrics.sample_count, 1);
if isempty(error_value)
    metrics.mae = NaN;
    metrics.rmse = NaN;
    metrics.max_abs_error = NaN;
    metrics.bias = NaN;
else
    metrics.mae = mean(abs(error_value));
    metrics.rmse = sqrt(mean(error_value.^2));
    metrics.max_abs_error = max(abs(error_value));
    metrics.bias = mean(error_value);
end
end
