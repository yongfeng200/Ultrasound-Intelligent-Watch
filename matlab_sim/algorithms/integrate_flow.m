function volume_m3 = integrate_flow(timestamp_s, flow_m3_s, valid, cfg)
%INTEGRATE_FLOW Integrate signed flow with the trapezoidal rule.

timestamp_s = timestamp_s(:);
flow_m3_s = flow_m3_s(:);
valid = logical(valid(:)) & isfinite(flow_m3_s);
if numel(timestamp_s) ~= numel(flow_m3_s) || numel(valid) ~= numel(flow_m3_s)
    error("Timestamp, flow, and validity vectors must have equal length.");
end
if any(diff(timestamp_s) <= 0)
    error("Timestamp must be strictly increasing.");
end

volume_m3 = zeros(size(flow_m3_s));
for k = 2:numel(flow_m3_s)
    dt = timestamp_s(k) - timestamp_s(k - 1);
    switch lower(string(cfg.integration.invalid_policy))
        case "skip"
            if valid(k) && valid(k - 1)
                volume_m3(k) = volume_m3(k - 1) + ...
                    0.5 * (flow_m3_s(k) + flow_m3_s(k - 1)) * dt;
            else
                volume_m3(k) = volume_m3(k - 1);
            end
        case "hold"
            q1 = flow_m3_s(k - 1);
            q2 = flow_m3_s(k);
            if ~valid(k - 1), q1 = 0; end
            if ~valid(k), q2 = q1; end
            volume_m3(k) = volume_m3(k - 1) + 0.5 * (q1 + q2) * dt;
        case "interpolate"
            q = flow_m3_s;
            q(~valid) = NaN;
            known = find(isfinite(q));
            if numel(known) < 2
                volume_m3(k) = volume_m3(k - 1);
            else
                q = interp1(timestamp_s(known), q(known), timestamp_s, "linear", "extrap");
                volume_m3(k) = volume_m3(k - 1) + 0.5 * (q(k) + q(k - 1)) * dt;
            end
        otherwise
            error("Unknown invalid integration policy: %s", cfg.integration.invalid_policy);
    end
end
end
