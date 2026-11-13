#!/usr/bin/env python3
"""
LS1D 开发板 UART 数据采集脚本
用途：通过串口采集超声波测量数据并保存为 CSV 格式
"""

import serial
import csv
import time
from datetime import datetime
import argparse

def parse_measurement_line(line):
    """
    解析测量数据行
    假设格式：TOF_DOWN:67500,TOF_UP:68200,TEMP:20.5,VALID:3
    需要根据实际嵌入式输出格式调整
    """
    data = {}
    try:
        parts = line.strip().split(',')
        for part in parts:
            if ':' in part:
                key, value = part.split(':')
                data[key] = value
        return data
    except:
        return None

def capture_data(port, baudrate, duration_sec, output_file):
    """
    采集数据并保存

    Args:
        port: 串口号 (如 'COM3' 或 '/dev/ttyUSB0')
        baudrate: 波特率 (默认 115200)
        duration_sec: 采集时长（秒）
        output_file: 输出 CSV 文件路径
    """

    print(f"正在连接到 {port} @ {baudrate} baud...")

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print("串口连接成功")
        time.sleep(2)  # 等待串口稳定

        # 打开 CSV 文件
        with open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)

            # 写入表头
            writer.writerow([
                'timestamp_s', 'tof_down_ticks', 'tof_up_ticks',
                'temperature_c', 'valid_down', 'valid_up'
            ])

            start_time = time.time()
            sample_count = 0

            print(f"开始采集数据，持续 {duration_sec} 秒...")
            print("按 Ctrl+C 提前停止")

            while (time.time() - start_time) < duration_sec:
                try:
                    if ser.in_waiting > 0:
                        line = ser.readline().decode('utf-8', errors='ignore')

                        # 根据实际格式解析
                        # 这里需要根据你的嵌入式代码输出格式修改
                        if 'TOF' in line or 'MEASURE' in line:
                            data = parse_measurement_line(line)

                            if data:
                                timestamp = time.time() - start_time

                                # 提取字段（根据实际格式调整）
                                tof_down = data.get('TOF_DOWN', 0)
                                tof_up = data.get('TOF_UP', 0)
                                temp = data.get('TEMP', 20.0)
                                valid = int(data.get('VALID', 0))

                                valid_down = 1 if (valid & 0x01) else 0
                                valid_up = 1 if (valid & 0x02) else 0

                                # 写入 CSV
                                writer.writerow([
                                    f"{timestamp:.3f}",
                                    tof_down,
                                    tof_up,
                                    temp,
                                    valid_down,
                                    valid_up
                                ])

                                sample_count += 1

                                if sample_count % 10 == 0:
                                    print(f"已采集 {sample_count} 个样本...")

                        # 显示原始行（用于调试）
                        # print(line.strip())

                except KeyboardInterrupt:
                    print("\n用户中断采集")
                    break
                except Exception as e:
                    print(f"解析错误: {e}")
                    continue

            print(f"\n采集完成！共 {sample_count} 个样本")
            print(f"数据已保存到: {output_file}")

    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查：")
        print("  1. 串口号是否正确")
        print("  2. 设备是否已连接")
        print("  3. 串口是否被其他程序占用")

    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("串口已关闭")

def main():
    parser = argparse.ArgumentParser(description='LS1D 开发板数据采集')
    parser.add_argument('--port', type=str, default='COM3',
                        help='串口号 (Windows: COM3, Linux: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='波特率 (默认: 115200)')
    parser.add_argument('--duration', type=int, default=60,
                        help='采集时长（秒，默认: 60）')
    parser.add_argument('--output', type=str,
                        default='../captured_data/ls1d_measurement.csv',
                        help='输出文件路径')

    args = parser.parse_args()

    # 创建输出目录
    import os
    os.makedirs(os.path.dirname(args.output), exist_ok=True)

    capture_data(args.port, args.baud, args.duration, args.output)

if __name__ == '__main__':
    main()

"""
使用示例：

1. Windows:
   python capture_uart_data.py --port COM3 --duration 120

2. Linux:
   python capture_uart_data.py --port /dev/ttyUSB0 --duration 120

3. 自定义输出文件:
   python capture_uart_data.py --port COM3 --output my_test_data.csv
"""
