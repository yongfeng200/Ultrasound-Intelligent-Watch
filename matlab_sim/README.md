# MATLAB 仿真说明

本目录用于龙芯 LS1D 超声波智能水表的算法级仿真和真实测量数据回放。它不修改现有嵌入式代码；当前工程中的 [target/sonar.S](../target/sonar.S)、[driver/ls1d_std_measure.c](../driver/ls1d_std_measure.c) 和 [include/ls1d_std_measure.h](../include/ls1d_std_measure.h) 仅作为功能边界参考。

## 快速运行

在 MATLAB 当前目录切换到 `matlab_sim`，执行：

```matlab
results = run_demo();
run_regression_tests();
replay = replay_csv("data/sample_measurement.csv");
```

`run_demo` 会生成一段包含零流、正向流量阶跃和反向流量的工况，并输出：

- 正向/逆向传播时间；
- TDC 量化、偏置、抖动和异常状态；
- 原始与滤波后的流速；
- 瞬时流量；
- 累计体积；
- `results/simulation_samples.csv`；
- `results/simulation_summary.txt`。

如果 MATLAB 没有图形桌面，可把 `cfg.sim.plot_results` 设置为 `false`。

## 重要参数

示例参数集中在 `config/default_config.m`，包括：

- 管径 `pipe_diameter_m`；
- 声程长度 `sound_path_m`；
- 安装角 `angle_deg`；
- 参考声速和温度系数；
- TDC tick 周期、方向通道偏置、抖动和异常概率；
- 滤波方法和窗口长度；
- 无效点积分策略。

管径、声程、安装角、声速系数以及 TDC 标度目前都是示例或待标定参数，不能直接视为实际水表的最终参数。

## 物理模型

仿真使用：

```text
t_down = L / (c + v*cos(theta))
t_up   = L / (c - v*cos(theta))
```

速度恢复使用精确反解：

```text
v = L/(2*cos(theta)) * (1/t_down - 1/t_up)
```

内部统一采用 SI 单位。流量输出绘图时转换为 L/min，累计体积转换为 L。

## CSV 回放

推荐的宽表格式为：

```text
timestamp_s,tof_down_s,tof_up_s,temperature_c,flow_reference_lpm,valid_down,valid_up,status_down,status_up
```

也可以使用 `tof_down_ticks` 和 `tof_up_ticks`，加载器会使用 `cfg.tdc.tick_period_s` 转换为秒。`flow_reference_lpm` 是可选的标准流量计参考值，用于计算误差。

后续从 LS1D UART0 导出数据时，建议至少保留时间戳、正向/反向 TDC 原始值、校准值、温度、有效标志和状态码，然后转换成上述格式。

## 当前限制

- 模型假设管道内为一维平均流速；
- 流速剖面修正系数默认是 1；
- 声速温度关系使用线性示例模型；
- TDC 模型是可配置的抽象模型，不是对 `sonar.S` 所有固定点细节的逐指令复现；
- 尚未包含真实换能器波形、AFE、管壁反射和气泡传播模型；
- 真实工程参数必须通过零流、标准流量、多温度和正反向测试标定。

## 建议的后续步骤

1. 在嵌入式例程中打通一次超声测量并输出原始正反向数据。
2. 将 UART 日志转换为 CSV。
3. 使用 `replay_csv` 对真实数据运行同一算法链路。
4. 比较不同滤波窗口、异常值阈值和温度补偿参数。
5. 选择可移植到 LS1D 的低复杂度算法，再回到开发板实测验证。
