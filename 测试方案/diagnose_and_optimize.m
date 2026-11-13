% 算法优化诊断与调整工具
% 用途：对比实测数据与仿真，自动诊断问题并给出优化建议

function optimization_report = diagnose_and_optimize(real_data_csv, reference_flow_lpm)
%DIAGNOSE_AND_OPTIMIZE 诊断实测数据问题并给出优化建议
%
% 输入:
%   real_data_csv - 实测数据文件路径
%   reference_flow_lpm - 标准流量计读数 (L/min)
%
% 输出:
%   optimization_report - 诊断报告和优化建议

fprintf('=== 智能水表算法优化诊断工具 ===\n\n');

%% 1. 加载实测数据
fprintf('【步骤 1】加载实测数据...\n');
cfg = default_config();
replay = replay_csv(real_data_csv, cfg);

%% 2. 基本统计
fprintf('\n【步骤 2】基本统计分析\n');
stats = struct();
stats.total_samples = length(replay.timestamp_s);
stats.valid_samples = sum(replay.valid);
stats.valid_ratio = stats.valid_samples / stats.total_samples;
stats.mean_flow = mean(replay.flow_filtered_l_min(replay.valid));
stats.std_flow = std(replay.flow_filtered_l_min(replay.valid));
stats.cv = stats.std_flow / stats.mean_flow;  % 变异系数

fprintf('总样本数: %d\n', stats.total_samples);
fprintf('有效样本: %d (%.2f%%)\n', stats.valid_samples, stats.valid_ratio*100);
fprintf('平均流量: %.3f L/min\n', stats.mean_flow);
fprintf('标准差: %.3f L/min\n', stats.std_flow);
fprintf('变异系数: %.2f%%\n', stats.cv*100);

if nargin >= 2 && ~isempty(reference_flow_lpm)
    stats.reference = reference_flow_lpm;
    stats.error = stats.mean_flow - reference_flow_lpm;
    stats.error_percent = (stats.error / reference_flow_lpm) * 100;
    fprintf('标准流量: %.3f L/min\n', reference_flow_lpm);
    fprintf('测量误差: %.3f L/min (%.2f%%)\n', stats.error, stats.error_percent);
end

%% 3. 问题诊断
fprintf('\n【步骤 3】问题诊断\n');
problems = {};
recommendations = {};

% 问题 1: 有效率低
if stats.valid_ratio < 0.85
    problems{end+1} = sprintf('有效率过低 (%.2f%%)', stats.valid_ratio*100);
    recommendations{end+1} = '优化目标: TDC 门控窗口、超时阈值';
    fprintf('❌ 问题 1: 有效率过低 (%.2f%% < 85%%)\n', stats.valid_ratio*100);
    fprintf('   可能原因:\n');
    fprintf('   - TDC 门控窗口设置不当\n');
    fprintf('   - 信号质量差\n');
    fprintf('   - 超时阈值过严\n');
end

% 问题 2: 变异系数大（波动大）
if stats.cv > 0.05  % 变异系数 > 5%
    problems{end+1} = sprintf('测量波动大 (CV=%.2f%%)', stats.cv*100);
    recommendations{end+1} = '优化目标: 滤波算法、窗口大小';
    fprintf('❌ 问题 2: 测量波动过大 (CV=%.2f%% > 5%%)\n', stats.cv*100);
    fprintf('   可能原因:\n');
    fprintf('   - 滤波不充分\n');
    fprintf('   - 异常值检测不够\n');
    fprintf('   - TDC 噪声大\n');
end

% 问题 3: 系统偏差大
if nargin >= 2 && abs(stats.error_percent) > 2.0
    problems{end+1} = sprintf('系统偏差大 (%.2f%%)', stats.error_percent);
    recommendations{end+1} = '优化目标: 零流标定、物理参数';
    fprintf('❌ 问题 3: 系统偏差过大 (%.2f%% > 2%%)\n', stats.error_percent);
    fprintf('   可能原因:\n');
    fprintf('   - 零流标定不准确\n');
    fprintf('   - 物理参数错误（管径、声程、角度）\n');
    fprintf('   - TDC tick 周期不对\n');
    fprintf('   - 流速剖面系数需调整\n');
end

% 问题 4: 异常值多
tof_diff = replay.tof_down_s - replay.tof_up_s;
valid_diff = tof_diff(replay.valid);
outlier_ratio = sum(abs(valid_diff - median(valid_diff)) > 3*std(valid_diff)) / length(valid_diff);
if outlier_ratio > 0.05
    problems{end+1} = sprintf('异常值比例高 (%.2f%%)', outlier_ratio*100);
    recommendations{end+1} = '优化目标: 异常检测阈值';
    fprintf('❌ 问题 4: 异常值比例高 (%.2f%% > 5%%)\n', outlier_ratio*100);
    fprintf('   可能原因:\n');
    fprintf('   - Hampel 阈值过宽\n');
    fprintf('   - 环境干扰大\n');
end

if isempty(problems)
    fprintf('✅ 未发现明显问题！\n');
end

%% 4. 优化建议矩阵
fprintf('\n【步骤 4】优化建议矩阵\n');
fprintf('┌─────────────────────┬──────────────────────────┐\n');
fprintf('│ 问题类型            │ 需要优化的算法/参数      │\n');
fprintf('├─────────────────────┼──────────────────────────┤\n');
fprintf('│ 有效率低 (<85%%)     │ ① TDC 门控窗口           │\n');
fprintf('│                     │ ② 超时检测阈值           │\n');
fprintf('├─────────────────────┼──────────────────────────┤\n');
fprintf('│ 波动大 (CV>5%%)      │ ① 滤波算法选择           │\n');
fprintf('│                     │ ② 滤波窗口大小           │\n');
fprintf('│                     │ ③ IIR 平滑系数           │\n');
fprintf('├─────────────────────┼──────────────────────────┤\n');
fprintf('│ 系统偏差 (>2%%)      │ ① 零流偏置补偿           │\n');
fprintf('│                     │ ② 物理参数标定           │\n');
fprintf('│                     │ ③ 流速剖面系数           │\n');
fprintf('├─────────────────────┼──────────────────────────┤\n');
fprintf('│ 异常值多 (>5%%)      │ ① Hampel 阈值            │\n');
fprintf('│                     │ ② 最大速度变化限制       │\n');
fprintf('└─────────────────────┴──────────────────────────┘\n');

%% 5. 自动优化尝试
fprintf('\n【步骤 5】自动参数优化\n');
fprintf('正在尝试不同参数组合...\n\n');

% 优化 1: 滤波窗口大小
if stats.cv > 0.05
    fprintf('>>> 优化 1: 滤波窗口大小\n');
    test_windows = [3, 5, 7, 9, 11];
    results = zeros(length(test_windows), 3);

    for i = 1:length(test_windows)
        cfg_test = cfg;
        cfg_test.filter.window_length = test_windows(i);
        replay_test = replay_csv(real_data_csv, cfg_test);

        valid_flow = replay_test.flow_filtered_l_min(replay_test.valid);
        results(i, 1) = test_windows(i);
        results(i, 2) = std(valid_flow);
        results(i, 3) = std(valid_flow) / mean(valid_flow) * 100;
    end

    fprintf('窗口大小  标准差(L/min)  变异系数(%%)\n');
    fprintf('────────  ─────────────  ───────────\n');
    for i = 1:length(test_windows)
        marker = '';
        if results(i, 2) == min(results(:, 2))
            marker = ' ← 推荐';
        end
        fprintf('   %2d        %.4f          %.2f%%%s\n', ...
            results(i, 1), results(i, 2), results(i, 3), marker);
    end

    [~, best_idx] = min(results(:, 2));
    fprintf('\n✅ 建议: 使用窗口大小 = %d\n', test_windows(best_idx));
    fprintf('   更新配置: cfg.filter.window_length = %d;\n\n', test_windows(best_idx));
end

% 优化 2: 滤波方法对比
fprintf('>>> 优化 2: 滤波方法对比\n');
filter_methods = ["median", "iir", "mean"];
filter_results = zeros(length(filter_methods), 3);

for i = 1:length(filter_methods)
    cfg_test = cfg;
    cfg_test.filter.method = filter_methods(i);
    replay_test = replay_csv(real_data_csv, cfg_test);

    valid_flow = replay_test.flow_filtered_l_min(replay_test.valid);
    filter_results(i, 1) = mean(valid_flow);
    filter_results(i, 2) = std(valid_flow);
    filter_results(i, 3) = std(valid_flow) / mean(valid_flow) * 100;
end

fprintf('方法      平均(L/min)  标准差(L/min)  变异系数(%%)\n');
fprintf('────────  ───────────  ─────────────  ───────────\n');
for i = 1:length(filter_methods)
    marker = '';
    if filter_results(i, 2) == min(filter_results(:, 2))
        marker = ' ← 推荐';
    end
    fprintf('%-8s    %.4f        %.4f          %.2f%%%s\n', ...
        filter_methods(i), filter_results(i, 1), ...
        filter_results(i, 2), filter_results(i, 3), marker);
end

[~, best_method_idx] = min(filter_results(:, 2));
fprintf('\n✅ 建议: 使用滤波方法 = "%s"\n', filter_methods(best_method_idx));
fprintf('   更新配置: cfg.filter.method = "%s";\n\n', filter_methods(best_method_idx));

% 优化 3: IIR 平滑系数（如果选用 IIR）
if filter_methods(best_method_idx) == "iir"
    fprintf('>>> 优化 3: IIR 平滑系数\n');
    test_alphas = [0.05, 0.10, 0.15, 0.20, 0.30];
    iir_results = zeros(length(test_alphas), 3);

    for i = 1:length(test_alphas)
        cfg_test = cfg;
        cfg_test.filter.method = "iir";
        cfg_test.filter.iir_alpha = test_alphas(i);
        replay_test = replay_csv(real_data_csv, cfg_test);

        valid_flow = replay_test.flow_filtered_l_min(replay_test.valid);
        iir_results(i, 1) = test_alphas(i);
        iir_results(i, 2) = std(valid_flow);
        iir_results(i, 3) = std(valid_flow) / mean(valid_flow) * 100;
    end

    fprintf('Alpha    标准差(L/min)  变异系数(%%)\n');
    fprintf('───────  ─────────────  ───────────\n');
    for i = 1:length(test_alphas)
        marker = '';
        if iir_results(i, 2) == min(iir_results(:, 2))
            marker = ' ← 推荐';
        end
        fprintf('%.2f        %.4f          %.2f%%%s\n', ...
            iir_results(i, 1), iir_results(i, 2), iir_results(i, 3), marker);
    end

    [~, best_alpha_idx] = min(iir_results(:, 2));
    fprintf('\n✅ 建议: 使用 alpha = %.2f\n', test_alphas(best_alpha_idx));
    fprintf('   更新配置: cfg.filter.iir_alpha = %.2f;\n\n', test_alphas(best_alpha_idx));
end

%% 6. 生成优化后的配置文件
fprintf('\n【步骤 6】生成优化配置\n');
optimized_cfg = cfg;

if stats.cv > 0.05 && exist('best_idx', 'var')
    optimized_cfg.filter.window_length = test_windows(best_idx);
end

if exist('best_method_idx', 'var')
    optimized_cfg.filter.method = filter_methods(best_method_idx);
end

if exist('best_alpha_idx', 'var') && filter_methods(best_method_idx) == "iir"
    optimized_cfg.filter.iir_alpha = test_alphas(best_alpha_idx);
end

% 保存优化配置
config_file = fullfile('..', 'calibration_data', 'optimized_config.m');
write_optimized_config(optimized_cfg, config_file);
fprintf('✅ 优化配置已保存到: %s\n', config_file);

%% 7. 验证优化效果
fprintf('\n【步骤 7】验证优化效果\n');
replay_optimized = replay_csv(real_data_csv, optimized_cfg);

fprintf('┌──────────────┬─────────┬─────────┬────────┐\n');
fprintf('│ 指标         │ 优化前  │ 优化后  │ 改善   │\n');
fprintf('├──────────────┼─────────┼─────────┼────────┤\n');

% 标准差对比
std_before = stats.std_flow;
std_after = std(replay_optimized.flow_filtered_l_min(replay_optimized.valid));
std_improve = (std_before - std_after) / std_before * 100;
fprintf('│ 标准差(L/min)│ %.4f  │ %.4f  │%+.1f%% │\n', ...
    std_before, std_after, std_improve);

% 变异系数对比
cv_before = stats.cv * 100;
cv_after = std_after / mean(replay_optimized.flow_filtered_l_min(replay_optimized.valid)) * 100;
cv_improve = (cv_before - cv_after) / cv_before * 100;
fprintf('│ 变异系数(%%) │ %.2f    │ %.2f    │%+.1f%% │\n', ...
    cv_before, cv_after, cv_improve);

% 有效率对比
valid_before = stats.valid_ratio * 100;
valid_after = sum(replay_optimized.valid) / length(replay_optimized.valid) * 100;
valid_improve = valid_after - valid_before;
fprintf('│ 有效率(%%)    │ %.2f    │ %.2f    │%+.1f%% │\n', ...
    valid_before, valid_after, valid_improve);

fprintf('└──────────────┴─────────┴─────────┴────────┘\n');

%% 8. 最终建议
fprintf('\n【步骤 8】最终优化建议\n');
fprintf('════════════════════════════════════════════\n\n');

if ~isempty(problems)
    fprintf('🔧 发现的问题:\n');
    for i = 1:length(problems)
        fprintf('   %d. %s\n', i, problems{i});
    end
    fprintf('\n');
end

fprintf('📋 推荐的配置参数:\n\n');
fprintf('function cfg = optimized_config()\n');
fprintf('    cfg = default_config();\n');
fprintf('    \n');
fprintf('    %% 滤波优化\n');
if exist('best_idx', 'var')
    fprintf('    cfg.filter.window_length = %d;  %% 优化窗口大小\n', test_windows(best_idx));
end
if exist('best_method_idx', 'var')
    fprintf('    cfg.filter.method = "%s";  %% 优化滤波方法\n', filter_methods(best_method_idx));
end
if exist('best_alpha_idx', 'var')
    fprintf('    cfg.filter.iir_alpha = %.2f;  %% 优化平滑系数\n', test_alphas(best_alpha_idx));
end

fprintf('    \n');
fprintf('    %% 其他需要手动标定的参数:\n');
if nargin >= 2 && abs(stats.error_percent) > 2.0
    fprintf('    %% cfg.tdc.offset_down_s = ?;  %% 需零流标定\n');
    fprintf('    %% cfg.tdc.offset_up_s = ?;    %% 需零流标定\n');
    fprintf('    %% cfg.physical.flow_profile_factor = ?;  %% 需多点标定\n');
end
fprintf('end\n\n');

fprintf('🎯 下一步行动:\n');
fprintf('   1. 使用优化后的配置重新测试\n');
fprintf('   2. 如果系统偏差仍大，执行零流标定和多点标定\n');
fprintf('   3. 记录优化前后的对比数据\n');
fprintf('   4. 更新嵌入式代码配置\n\n');

fprintf('════════════════════════════════════════════\n');
fprintf('诊断完成！\n');

%% 输出结果
optimization_report = struct();
optimization_report.stats = stats;
optimization_report.problems = problems;
optimization_report.recommendations = recommendations;
optimization_report.optimized_cfg = optimized_cfg;

end

function write_optimized_config(cfg, filename)
% 写入优化后的配置文件
fid = fopen(filename, 'w');
fprintf(fid, 'function cfg = optimized_config()\n');
fprintf(fid, '%% 自动优化生成的配置\n');
fprintf(fid, '%% 生成时间: %s\n\n', datestr(now));
fprintf(fid, 'cfg = default_config();\n\n');
fprintf(fid, '%% 滤波优化参数\n');
fprintf(fid, 'cfg.filter.method = "%s";\n', cfg.filter.method);
fprintf(fid, 'cfg.filter.window_length = %d;\n', cfg.filter.window_length);
fprintf(fid, 'cfg.filter.iir_alpha = %.2f;\n\n', cfg.filter.iir_alpha);
fprintf(fid, 'end\n');
fclose(fid);
end
