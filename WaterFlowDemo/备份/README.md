# LS1D 水流速展示终端

C# / WPF 桌面展示程序。显示顺流分析时间、逆流分析时间、时间差和计算水流速度，支持修改数据和恢复示例。当前使用模拟传播时间，没有连接芯片或串口。

## 运行

双击 `发布版/WaterFlowDemo.exe`。运行环境：Windows 和 .NET 8 Desktop Runtime。

源码项目：`WaterFlowDemo.csproj`，可以用 Visual Studio 的 .NET 桌面开发工作负载打开。

```powershell
dotnet build -c Release
dotnet publish -c Release -o 发布版
```

## 计算说明

`v = L / (2 × cos θ) × (1/T_f − 1/T_r)`。计算时将微秒转换为秒，毫米转换为米。正值表示正向流动，负值表示反向流动。

示例：T_f = 27.001 μs，T_r = 27.053 μs，L = 40 mm，θ = 45°。时差为 52 ns，流速约 2.014 m/s。声程和角度是展示假设，不代表已确认的硬件尺寸。

参考工作区 `13、LS1D-DEV-013-龙芯超声波水表方案介绍.pdf` 的时差法原理（已有文本在 `tmp/research/_src/ls1d-watermeter.clean.txt`），以及 `ai-team/ppt/script.md` 的公式与示例参数。这里的时间指声波传播时间，不是芯片程序处理耗时。实际硬件接入还需依据通信协议解码并进行标定。
