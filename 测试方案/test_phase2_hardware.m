% 阶段二：硬件集成测试
% 前提：LS1D 开发板已能通过 UART 输出测量数据
% 目的：验证 MATLAB 算法与嵌入式实现的一致性

%% 准备工作
cd('D:\WorkSpace2\上海电机学院\龙芯智能水表\5、LS1D-DEV-005-基于VScode的开发板例程\demo-window\demo-window\1.gpio_led_control_demo\matlab_sim');

%% 测试 2.1：加载示例测量数据
fprintf('=== 测试 2.1: CSV 数据回放 ===\n');
try
    replay = replay_csv("data/sample_measurement.csv");
    fprintf('成功加载示例数据\n');
    fprintf('数据点数: %d\n', length(replay.timestamp_s));
catch ME
    fprintf('示例数据加载失败: %s\n', ME.message);
end

%% 测试 2.2：UART 数据采集指南
fprintf('\n=== 测试 2.2: UART 数据采集 ===\n');
fprintf('【硬件操作步骤】\n');
fprintf('1. 连接 LS1D 开发板到 PC (UART0, 115200 baud)\n');
fprintf('2. 运行嵌入式测量程序\n');
fprintf('3. 使用串口工具或脚本采集数据\n');
fprintf('4. 保存为 CSV 格式\n\n');

fprintf('【推荐 CSV 格式】\n');
fprintf('timestamp_s,tof_down_ticks,tof_up_ticks,temperature_c,valid_down,valid_up\n');
fprintf('0.000,67500,68200,20.5,1,1\n');
fprintf('0.250,67505,68195,20.5,1,1\n');
fprintf('...\n\n');

fprintf('【Python 串口采集示例】\n');
fprintf('见: capture_uart_data.py\n\n');

%% 测试 2.3：真实数据回放（需要先采集数据）
fprintf('=== 测试 2.3: 真实数据回放 ===\n');
real_data_file = fullfile('..', 'captured_data', 'ls1d_measurement.csv');
if exist(real_data_file, 'file')
    fprintf('发现真实数据文件，开始回放...\n');
    cfg = default_config();
    % 根据实际 TDC 参数调整
    cfg.tdc.tick_period_s = 1e-9;  % 需要根据实测确认

    replay = replay_csv(real_data_file, cfg);

    fprintf('数据点数: %d\n', length(replay.timestamp_s));
    fprintf('有效率: %.2f%%\n', replay.metrics.valid_ratio * 100);
    fprintf('平均流速: %.6f m/s\n', mean(replay.velocity_filtered_m_s(replay.valid)));
    fprintf('平均流量: %.3f L/min\n', mean(replay.flow_filtered_l_min(replay.valid)));
else
    fprintf('未找到真实数据文件: %s\n', real_data_file);
    fprintf('请先使用开发板采集数据\n');
end

%% 测试 2.4：Golden Vector 对比
fprintf('\n=== 测试 2.4: Golden Vector 验证 ===\n');
fprintf('【目的】验证 MATLAB 和嵌入式代码的计算一致性\n\n');

% 生成标准测试向量
cfg = default_config();
cfg.sim.duration_s = 10;
cfg.sim.sample_period_s = 1.0;
cfg.sim.random_seed = 12345;  % 固定种子保证可重复
cfg.sim.plot_results = false;

results = run_demo(cfg);

% 导出 golden vector
golden_file = fullfile('..', 'golden_vectors', 'matlab_golden.csv');
golden_dir = fileparts(golden_file);
if ~exist(golden_dir, 'dir')
    mkdir(golden_dir);
end

golden_data = table();
golden_data.timestamp_s = results.timestamp_s;
golden_data.tof_down_ticks = results.tof_down_ticks;
golden_data.tof_up_ticks = results.tof_up_ticks;
golden_data.velocity_expected_m_s = results.velocity_filtered_m_s;
golden_data.flow_expected_l_min = results.flow_filtered_l_min;
golden_data.valid_expected = results.valid;

writetable(golden_data, golden_file);
fprintf('已生成 Golden Vector: %s\n', golden_file);
fprintf('请将此文件用于嵌入式代码的单元测试\n\n');

fprintf('【下一步】\n');
fprintf('1. 将 tof_down_ticks, tof_up_ticks 输入到嵌入式代码\n');
fprintf('2. 记录嵌入式代码计算的 velocity 和 flow\n');
fprintf('3. 与 velocity_expected, flow_expected 对比\n');
fprintf('4. 误差应在定点精度范围内（如 <0.1%%）\n');

fprintf('\n阶段二测试完成！\n');
