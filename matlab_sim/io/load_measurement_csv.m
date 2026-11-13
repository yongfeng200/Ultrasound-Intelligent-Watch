function data = load_measurement_csv(file_path, cfg)
%LOAD_MEASUREMENT_CSV Load a normalized wide-format measurement CSV.
if ~isfile(file_path)
    error("Measurement file does not exist: %s", file_path);
end

table_data = readtable(file_path, "TextType", "string");
required = "timestamp_s";
if ~ismember(required, string(table_data.Properties.VariableNames))
    error("CSV must contain timestamp_s.");
end

data.timestamp_s = table_data.timestamp_s(:);
n = numel(data.timestamp_s);
data.tof_down_s = column_or_nan(table_data, "tof_down_s", n);
data.tof_up_s = column_or_nan(table_data, "tof_up_s", n);
data.tof_down_ticks = column_or_nan(table_data, "tof_down_ticks", n);
data.tof_up_ticks = column_or_nan(table_data, "tof_up_ticks", n);
data.temperature_c = column_or_nan(table_data, "temperature_c", n);
data.flow_reference_lpm = column_or_nan(table_data, "flow_reference_lpm", n);
data.valid_down = logical(column_or_default(table_data, "valid_down", n, true));
data.valid_up = logical(column_or_default(table_data, "valid_up", n, true));
data.status_down = string(column_or_default(table_data, "status_down", n, "VALID"));
data.status_up = string(column_or_default(table_data, "status_up", n, "VALID"));

if all(isnan(data.tof_down_s)) && any(isfinite(data.tof_down_ticks))
    data.tof_down_s = data.tof_down_ticks .* cfg.tdc.tick_period_s;
end
if all(isnan(data.tof_up_s)) && any(isfinite(data.tof_up_ticks))
    data.tof_up_s = data.tof_up_ticks .* cfg.tdc.tick_period_s;
end
if any(~isfinite(data.timestamp_s)) || any(diff(data.timestamp_s) <= 0)
    error("timestamp_s must be finite and strictly increasing.");
end
end

function value = column_or_nan(table_data, name, n)
if ismember(name, string(table_data.Properties.VariableNames))
    value = double(table_data.(name));
else
    value = nan(n, 1);
end
end

function value = column_or_default(table_data, name, n, default_value)
if ismember(name, string(table_data.Properties.VariableNames))
    value = table_data.(name);
else
    if isstring(default_value)
        value = repmat(default_value, n, 1);
    else
        value = repmat(default_value, n, 1);
    end
end
end
