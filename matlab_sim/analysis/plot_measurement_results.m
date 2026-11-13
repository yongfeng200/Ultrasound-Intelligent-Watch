function plot_measurement_results(results)
%PLOT_MEASUREMENT_RESULTS Plot the main simulated meter signals.
figure("Name", "LS1D ultrasonic water-meter simulation", "Color", "w");

subplot(4, 1, 1);
plot(results.timestamp_s, results.velocity_true_m_s, "k--", "LineWidth", 1.2); hold on;
plot(results.timestamp_s, results.velocity_raw_m_s, "Color", [0.75 0.75 0.75]);
plot(results.timestamp_s, results.velocity_filtered_m_s, "b", "LineWidth", 1.1);
grid on; ylabel("Velocity (m/s)");
legend("True", "Raw", "Filtered", "Location", "best");

subplot(4, 1, 2);
plot(results.timestamp_s, results.flow_true_l_min, "k--", "LineWidth", 1.2); hold on;
plot(results.timestamp_s, results.flow_filtered_l_min, "r", "LineWidth", 1.1);
grid on; ylabel("Flow (L/min)");
legend("True", "Estimated", "Location", "best");

subplot(4, 1, 3);
plot(results.timestamp_s, results.tof_down_us, "b"); hold on;
plot(results.timestamp_s, results.tof_up_us, "r");
grid on; ylabel("TOF (us)");
legend("Downstream", "Upstream", "Location", "best");

subplot(4, 1, 4);
plot(results.timestamp_s, results.volume_l, "m", "LineWidth", 1.1);
grid on; xlabel("Time (s)"); ylabel("Volume (L)");
end
