% 阶段三：实流标定与验证测试
% 前提：有标准流量计和可控水流测试台
% 目的：标定物理参数，验证测量精度

%% 测试 3.1：零流测试
fprintf('=== 测试 3.1: 零流标定 ===\n');
fprintf('【测试目的】确定 TDC 通道偏置\n\n');

fprintf('【测试步骤】\n');
fprintf('1. 关闭水流，确保管道内静止\n');
fprintf('2. 采集至少 5 分钟数据（300 秒 @ 0.25s 采样 = 1200 个样本）\n');
fprintf('3. 保存为 zero_flow_test.csv\n\n');

% 如果已有零流数据
zero_flow_file = fullfile('..', 'calibration_data', 'zero_flow_test.csv');
if exist(zero_flow_file, 'file')
    fprintf('发现零流测试数据，开始分析...\n');
    data = readtable(zero_flow_file);

    % 计算平均 TOF
    tof_down_mean = mean(data.tof_down_ticks);
    tof_up_mean = mean(data.tof_up_ticks);
    tof_diff = tof_down_mean - tof_up_mean;

    fprintf('平均下行 TOF: %.2f ticks\n', tof_down_mean);
    fprintf('平均上行 TOF: %.2f ticks\n', tof_up_mean);
    fprintf('平均时差: %.2f ticks\n', tof_diff);

    % 理想情况下，零流时 tof_down 应该等于 tof_up
    % 实际偏差即为通道偏置
    cfg = default_config();
    offset_s = tof_diff * cfg.tdc.tick_period_s;
    fprintf('建议偏置补偿: %.2f ns\n', offset_s * 1e9);
    fprintf('更新 config: cfg.tdc.offset_down_s = %.2e;\n\n', offset_s/2);
else
    fprintf('未找到零流数据: %s\n', zero_flow_file);
    fprintf('请先进行零流测试\n\n');
end

%% 测试 3.2：标准流量标定
fprintf('=== 测试 3.2: 标准流量标定 ===\n');
fprintf('【测试目的】验证测量精度，调整流速剖面系数\n\n');

fprintf('【测试步骤】\n');
fprintf('1. 设置标准流量计（参考值）\n');
fprintf('2. 对多个流量点进行测试（建议: 0.5, 1.0, 2.0, 5.0 L/min）\n');
fprintf('3. 每个流量点采集 3-5 分钟数据\n');
fprintf('4. 保存为 flow_X_lpm.csv\n\n');

% 标定流量点
flow_points = [0.5, 1.0, 2.0, 5.0];  % L/min
calibration_results = struct();

for i = 1:length(flow_points)
    flow_lpm = flow_points(i);
    test_file = fullfile('..', 'calibration_data', ...
        sprintf('flow_%.1f_lpm.csv', flow_lpm));

    if exist(test_file, 'file')
        fprintf('分析流量点 %.1f L/min...\n', flow_lpm);

        cfg = default_config();
        % 应用零流标定结果
        % cfg.tdc.offset_down_s = xxx;  % 从测试 3.1 获取

        replay = replay_csv(test_file, cfg);

        measured_flow = mean(replay.flow_filtered_l_min(replay.valid));
        error_percent = (measured_flow - flow_lpm) / flow_lpm * 100;

        fprintf('  参考流量: %.3f L/min\n', flow_lpm);
        fprintf('  测量流量: %.3f L/min\n', measured_flow);
        fprintf('  相对误差: %.2f%%\n', error_percent);

        calibration_results(i).reference = flow_lpm;
        calibration_results(i).measured = measured_flow;
        calibration_results(i).error_percent = error_percent;
    else
        fprintf('未找到流量点 %.1f L/min 的数据\n', flow_lpm);
    end
end

% 如果有多个点的数据，计算线性拟合
if exist('calibration_results', 'var') && length(calibration_results) >= 2
    refs = [calibration_results.reference];
    meas = [calibration_results.measured];

    % 线性拟合
    p = polyfit(refs, meas, 1);

    fprintf('\n线性拟合结果:\n');
    fprintf('  y = %.4f * x + %.4f\n', p(1), p(2));
    fprintf('  建议校正系数: %.4f\n', 1/p(1));
    fprintf('  更新 config: cfg.physical.flow_profile_factor = %.4f;\n\n', 1/p(1));
end

%% 测试 3.3：温度补偿验证
fprintf('=== 测试 3.3: 温度补偿验证 ===\n');
fprintf('【测试目的】验证声速-温度模型\n\n');

fprintf('【测试步骤】\n');
fprintf('1. 在不同水温下（如 10°C, 20°C, 30°C）测试相同流量\n');
fprintf('2. 每个温度点采集 3 分钟数据\n');
fprintf('3. 观察流速测量是否稳定\n\n');

temp_points = [10, 20, 30];  % °C
fixed_flow_lpm = 2.0;

for i = 1:length(temp_points)
    temp_c = temp_points(i);
    test_file = fullfile('..', 'calibration_data', ...
        sprintf('temp_%d_flow_%.1f.csv', temp_c, fixed_flow_lpm));

    if exist(test_file, 'file')
        fprintf('分析温度点 %d°C...\n', temp_c);

        cfg = default_config();
        replay = replay_csv(test_file, cfg);

        measured_flow = mean(replay.flow_filtered_l_min(replay.valid));
        flow_std = std(replay.flow_filtered_l_min(replay.valid));

        fprintf('  测量流量: %.3f ± %.3f L/min\n', measured_flow, flow_std);
        fprintf('  相对标准差: %.2f%%\n', flow_std/measured_flow*100);
    else
        fprintf('未找到温度点 %d°C 的数据\n', temp_c);
    end
end

%% 测试 3.4：长期稳定性测试
fprintf('\n=== 测试 3.4: 长期稳定性 ===\n');
fprintf('【测试目的】验证累计体积精度\n\n');

fprintf('【测试步骤】\n');
fprintf('1. 设置固定流量（如 2.0 L/min）\n');
fprintf('2. 连续运行 1 小时\n');
fprintf('3. 对比累计体积与参考值\n\n');

long_test_file = fullfile('..', 'calibration_data', 'long_term_test.csv');
if exist(long_test_file, 'file')
    fprintf('分析长期稳定性数据...\n');

    cfg = default_config();
    replay = replay_csv(long_test_file, cfg);

    duration_min = (replay.timestamp_s(end) - replay.timestamp_s(1)) / 60;
    total_volume_l = replay.volume_l(end);

    % 假设参考流量为 2.0 L/min
    reference_flow_lpm = 2.0;
    reference_volume_l = reference_flow_lpm * duration_min;

    error_l = total_volume_l - reference_volume_l;
    error_percent = error_l / reference_volume_l * 100;

    fprintf('测试时长: %.1f 分钟\n', duration_min);
    fprintf('参考累计: %.3f L\n', reference_volume_l);
    fprintf('测量累计: %.3f L\n', total_volume_l);
    fprintf('绝对误差: %.3f L\n', error_l);
    fprintf('相对误差: %.2f%%\n', error_percent);
else
    fprintf('未找到长期测试数据\n\n');
end

%% 生成标定报告
fprintf('\n=== 生成标定报告 ===\n');
report_file = fullfile('..', 'calibration_data', 'calibration_report.txt');
fid = fopen(report_file, 'w');
fprintf(fid, '龙芯智能水表标定报告\n');
fprintf(fid, '生成时间: %s\n\n', datestr(now));
fprintf(fid, '请根据实际测试结果填写参数\n');
fclose(fid);
fprintf('报告已保存到: %s\n', report_file);

fprintf('\n阶段三测试完成！\n');
fprintf('标定完成后，请更新 config/default_config.m 中的参数\n');
