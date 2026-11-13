# 嵌入式功能映射

| MATLAB 仿真环节 | LS1D 工程参考 | 当前关系 |
|---|---|---|
| TDC 原始值与时间转换 | `target/sonar.S` 中 `tdc_calcc`、`tdc2time`、`tdc1517_2ps` | MATLAB 使用显式 tick 周期参数；汇编中的固定点比例需结合芯片文档和实测数据进一步确认 |
| 正反向 TOF 处理 | `target/sonar.S` 中 `dtofcalc` | MATLAB 等价地计算顺流、逆流 TOF，再求时差和平均值 |
| 正反向结果合并 | `target/sonar.S` 中 `sfa` | MATLAB 首期直接使用两方向传播时间；SFA 的硬件定点行为可在取得 golden vector 后补充 |
| 测量门控窗口 | `driver/ls1d_std_measure.c` 中 `MEASURE_GateWindowInit` | MATLAB 暂时以有效测量和超时概率抽象，不逐位仿真寄存器 |
| 默认测量时序 | `driver/ls1d_std_measure.c` 中 `MEASURE_DefaultInit` | 用于理解发波数、测量周期和窗口配置；不会自动推断实际声道参数 |
| 测量寄存器与状态 | `include/ls1d_std_measure.h` | MATLAB 保留 valid/status 字段，后续可按实际 UART 输出扩展 |
| UART 数据采集 | `UserSrc/UserUart.c` | 当前 UART0 初始化为 115200；建议后续输出原始正反向 TDC、温度和状态字段 |

## 注意事项

1. 本 MATLAB 工程是算法级模型，不是 LS1D 汇编的逐指令模拟器。
2. `TDC_RES_DEFAULT` 和 `TDC_CAL_DEFAULT` 等工程常量不能直接替换成物理时间单位，除非已经确认寄存器标度和校准流程。
3. 后续应使用同一组输入数据构造嵌入式输出与 MATLAB 输出的 golden vector，逐样本比较 TDC 转换、时差、滤波和流量结果。
4. 在完成真实水流测试前，管径、声程、角度、声速模型、通道偏置和流速剖面修正系数都应视为待标定参数。
