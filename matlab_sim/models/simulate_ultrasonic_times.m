function [tof_down_s, tof_up_s, sound_speed_m_s] = ...
    simulate_ultrasonic_times(velocity_m_s, temperature_c, cfg)
%SIMULATE_ULTRASONIC_TIMES Simulate down/upstream ultrasonic TOF.

sound_speed_m_s = cfg.physical.reference_sound_speed_m_s + ...
    cfg.physical.sound_speed_slope_m_s_per_c .* ...
    (temperature_c - cfg.physical.reference_temperature_c);

cos_theta = cosd(cfg.physical.angle_deg);
if any(sound_speed_m_s <= 0, "all")
    error("Sound speed must be positive.");
end
if any(abs(velocity_m_s .* cos_theta) >= sound_speed_m_s, "all")
    error("Flow speed is outside the physical propagation range.");
end

tof_down_s = cfg.physical.sound_path_m ./ ...
    (sound_speed_m_s + velocity_m_s .* cos_theta);
tof_up_s = cfg.physical.sound_path_m ./ ...
    (sound_speed_m_s - velocity_m_s .* cos_theta);
end
