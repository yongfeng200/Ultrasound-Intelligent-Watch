function flow_m3_s = velocity_to_flow(velocity_m_s, cfg)
%VELOCITY_TO_FLOW Convert mean velocity to signed volumetric flow.
area_m2 = pi * cfg.physical.pipe_diameter_m^2 / 4;
flow_m3_s = velocity_m_s .* area_m2 .* cfg.physical.flow_profile_factor;
end
