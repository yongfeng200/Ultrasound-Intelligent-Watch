# 测试数据格式

## 推荐宽表

每一行表示一个同步测量时刻：

```text
timestamp_s,tof_down_s,tof_up_s,temperature_c,flow_reference_lpm,valid_down,valid_up,status_down,status_up
```

## 字段说明

| 字段 | 单位 | 必填 | 说明 |
|---|---:|---:|---|
| `timestamp_s` | s | 是 | 严格递增的采样时间 |
| `tof_down_s` | s | 二选一 | 顺流传播时间 |
| `tof_up_s` | s | 二选一 | 逆流传播时间 |
| `tof_down_ticks` | tick | 二选一 | 顺流 TDC 原始计数 |
| `tof_up_ticks` | tick | 二选一 | 逆流 TDC 原始计数 |
| `temperature_c` | °C | 否 | 测量点水温 |
| `flow_reference_lpm` | L/min | 否 | 标准流量计参考值 |
| `valid_down` | 0/1 | 否 | 顺流测量是否有效，默认 1 |
| `valid_up` | 0/1 | 否 | 逆流测量是否有效，默认 1 |
| `status_down` | 文本 | 否 | 顺流状态，默认 `VALID` |
| `status_up` | 文本 | 否 | 逆流状态，默认 `VALID` |

如果同时提供 `tof_*_s` 和 `tof_*_ticks`，当前加载器优先使用秒字段。仅提供 tick 时，使用 `config/default_config.m` 中的 `cfg.tdc.tick_period_s` 换算。

## 状态建议

建议使用以下状态值：

- `VALID`
- `TIMEOUT`
- `OUTLIER`
- `MISSING`
- `REJECTED_BY_FILTER`

原始状态应保留，滤波拒绝原因由 MATLAB 算法另外输出，不覆盖硬件状态。
