% 阶段一：MATLAB 仿真基础测试
% 目的：验证算法链路完整性，熟悉各模块功能

%% 测试 1：运行默认演示
cd('D:\WorkSpace2\上海电机学院\龙芯智能水表\5、LS1D-DEV-005-基于VScode的开发板例程\demo-window\demo-window\1.gpio_led_control_demo\matlab_sim');
fprintf('=== 测试 1: 运行默认演示 ===\n');
results = run_demo();

% 检查关键输出
fprintf('样本数量: %d\n', length(results.timestamp_s));
fprintf('有效率: %.2f%%\n', results.metrics.valid_ratio * 100);
fprintf('速度 MAE: %.6f m/s\n', results.metrics.mae);
fprintf('速度 RMSE: %.6f m/s\n', results.metrics.rmse);
fprintf('累计体积: %.3f L\n', results.volume_l(end));

%% 测试 2：参数敏感性分析
fprintf('\n=== 测试 2: 参数敏感性分析 ===\n');
cfg = default_config();
cfg.sim.plot_results = false;

% 测试不同滤波窗口
window_lengths = [3, 5, 7, 9];
for i = 1:length(window_lengths)
    cfg.filter.window_length = window_lengths(i);
    res = run_demo(cfg);
    fprintf('窗口长度 %d: MAE=%.6f, 有效率=%.2f%%\n', ...
        window_lengths(i), res.metrics.mae, res.metrics.valid_ratio*100);
end

% 测试不同滤波方法
filter_methods = ["median", "iir", "none"];
cfg.filter.window_length = 5;
for i = 1:length(filter_methods)
    cfg.filter.method = filter_methods(i);
    res = run_demo(cfg);
    fprintf('滤波方法 %s: MAE=%.6f, RMSE=%.6f\n', ...
        filter_methods(i), res.metrics.mae, res.metrics.rmse);
end

%% 测试 3：异常场景
fprintf('\n=== 测试 3: 异常场景测试 ===\n');
cfg = default_config();
cfg.sim.plot_results = false;

% 高噪声场景
cfg.tdc.jitter_std_s = 10e-9;
cfg.tdc.outlier_probability = 0.05;
res_noisy = run_demo(cfg);
fprintf('高噪声: MAE=%.6f, 有效率=%.2f%%\n', ...
    res_noisy.metrics.mae, res_noisy.metrics.valid_ratio*100);

% 频繁超时场景
cfg = default_config();
cfg.tdc.timeout_probability = 0.1;
res_timeout = run_demo(cfg);
fprintf('频繁超时: 有效率=%.2f%%\n', res_timeout.metrics.valid_ratio*100);

%% 测试 4：回归测试
fprintf('\n=== 测试 4: 运行回归测试 ===\n');
run_regression_tests();

fprintf('\n阶段一测试完成！\n');
fprintf('请检查 results/ 目录下生成的 CSV 和图表\n');
