function [filtered, accepted, reject_reason] = filter_measurement(values, valid, cfg)
%FILTER_MEASUREMENT Apply validity checks and a configurable robust filter.
values = values(:);
valid = logical(valid(:)) & isfinite(values);
filtered = nan(size(values));
accepted = false(size(values));
reject_reason = repmat("INVALID_INPUT", size(values));

if any(valid)
    median_value = moving_median(values, valid, cfg.filter.window_length);
    deviation = abs(values - median_value);
    scale = moving_mad(values, valid, cfg.filter.window_length);
    robust_limit = cfg.filter.hampel_threshold .* max(scale, eps);
    suspicious = valid & isfinite(median_value) & deviation > robust_limit;
    accepted = valid & ~suspicious;
    reject_reason(accepted) = "ACCEPTED";
    reject_reason(~valid) = "INVALID_INPUT";
    reject_reason(suspicious) = "REJECTED_OUTLIER";
else
    return;
end

switch lower(string(cfg.filter.method))
    case "median"
        filtered = moving_median(values, accepted, cfg.filter.window_length);
    case "mean"
        filtered = moving_mean(values, accepted, cfg.filter.window_length);
    case "iir"
        filtered = iir_filter(values, accepted, cfg.filter.iir_alpha);
    otherwise
        error("Unknown filter method: %s", cfg.filter.method);
end

filtered(~accepted) = NaN;
end

function output = moving_median(values, valid, window_length)
output = nan(size(values));
half_window = floor(window_length / 2);
for k = 1:numel(values)
    first = max(1, k - half_window);
    last = min(numel(values), k + half_window);
    window = values(first:last);
    window = window(valid(first:last) & isfinite(window));
    if ~isempty(window)
        output(k) = median(window);
    end
end
end

function output = moving_mean(values, valid, window_length)
output = nan(size(values));
half_window = floor(window_length / 2);
for k = 1:numel(values)
    first = max(1, k - half_window);
    last = min(numel(values), k + half_window);
    window = values(first:last);
    window = window(valid(first:last) & isfinite(window));
    if ~isempty(window)
        output(k) = mean(window);
    end
end
end

function output = moving_mad(values, valid, window_length)
output = nan(size(values));
half_window = floor(window_length / 2);
for k = 1:numel(values)
    first = max(1, k - half_window);
    last = min(numel(values), k + half_window);
    window = values(first:last);
    window = window(valid(first:last) & isfinite(window));
    if ~isempty(window)
        center = median(window);
        output(k) = median(abs(window - center));
    end
end
end

function output = iir_filter(values, valid, alpha)
output = nan(size(values));
previous = NaN;
for k = 1:numel(values)
    if ~valid(k)
        continue;
    end
    if isnan(previous)
        previous = values(k);
    else
        previous = alpha * values(k) + (1 - alpha) * previous;
    end
    output(k) = previous;
end
end
