function [t_s, velocity_m_s, temperature_c] = generate_flow_profile(cfg)
%GENERATE_FLOW_PROFILE Generate a reproducible piecewise water-flow profile.

t_s = (0:cfg.sim.sample_period_s:cfg.sim.duration_s).';
velocity_m_s = zeros(size(t_s));

% Zero flow, positive step, higher flow, then reverse flow.
velocity_m_s(t_s >= 5 & t_s < 12) = 0.20;
velocity_m_s(t_s >= 12 & t_s < 20) = 0.80;
velocity_m_s(t_s >= 20) = -0.35;

% Add a small ramp during the first transition to exercise the filter.
ramp = t_s >= 4 & t_s < 5;
velocity_m_s(ramp) = 0.20 * (t_s(ramp) - 4);

temperature_c = cfg.physical.reference_temperature_c + ...
    cfg.noise.temperature_std_c .* randn(size(t_s));
end
