function [velocity_m_s, valid] = time_to_velocity(tof_down_s, tof_up_s, cfg, method)
%TIME_TO_VELOCITY Recover signed mean flow velocity from two TOFs.
if nargin < 4
    method = "exact";
end

valid = isfinite(tof_down_s) & isfinite(tof_up_s) & ...
    tof_down_s > 0 & tof_up_s > 0;
velocity_m_s = nan(size(tof_down_s));
cos_theta = cosd(cfg.physical.angle_deg);

switch lower(string(method))
    case "exact"
        velocity_m_s(valid) = cfg.physical.sound_path_m ./ (2 * cos_theta) .* ...
            (1 ./ tof_down_s(valid) - 1 ./ tof_up_s(valid));
    case "approximate"
        c = cfg.physical.reference_sound_speed_m_s;
        velocity_m_s(valid) = c^2 ./ (2 * cfg.physical.sound_path_m * cos_theta) .* ...
            (tof_up_s(valid) - tof_down_s(valid));
    otherwise
        error("Unknown velocity method: %s", method);
end

valid = valid & abs(velocity_m_s) < 100;
velocity_m_s(~valid) = NaN;
end
