using System;
using System.Globalization;
using System.IO;

namespace WaterFlowDemo
{
    /// <summary>一次温度上报的解析结果。</summary>
    public sealed class TemperatureReading
    {
        public bool HasValue { get; set; }     // 直接上报了 °C
        public double ValueC { get; set; }
        public bool HasRaw { get; set; }       // 只上报了原始计数值（resF）
        public long Raw { get; set; }
        public bool Timeout { get; set; }      // TEMP,timeout
        public bool Marker { get; set; }       // TEMPCODE,v1
    }

    /// <summary>
    /// 水的声速与温度补偿流速。
    ///   c(T) 用 Mackenzie(1981) 经验公式（0~100°C，误差 &lt;0.1 m/s）
    ///   温度法：v = c(T) · (Tr − Tf) / (Tr + Tf) / cosθ
    /// 推导：1/Tf − 1/Tr = 2v·cosθ/L，1/Tf + 1/Tr = 2c/L，两式相除即可消掉 L。
    /// </summary>
    public static class WaterTemperature
    {
        public static double SoundSpeedMs(double tempC)
        {
            if (!FlowMath.IsFinite(tempC) || tempC < -20.0 || tempC > 120.0)
            {
                return double.NaN;
            }
            double t = tempC;
            return 1402.388
                + 5.03830 * t
                - 5.81090e-2 * t * t
                + 3.3432e-4 * t * t * t
                - 1.47797e-6 * t * t * t * t
                + 3.1419e-9 * t * t * t * t * t;
        }

        /// <summary>由声速和两个飞行时间反推声程 L（mm）：L = c·(Tf'+Tr')/2，Tf' = Tf − d。</summary>
        public static double PathMm(double soundSpeedMs, double forwardUs, double reverseUs, double delayUs)
        {
            if (!FlowMath.IsFinite(soundSpeedMs) || soundSpeedMs <= 0
                || !FlowMath.IsFinite(forwardUs) || !FlowMath.IsFinite(reverseUs) || !FlowMath.IsFinite(delayUs)
                || forwardUs <= 0 || reverseUs <= 0 || delayUs < 0)
            {
                return double.NaN;
            }
            double sum = (forwardUs - delayUs) + (reverseUs - delayUs);
            if (sum <= 0)
            {
                return double.NaN;
            }
            return soundSpeedMs * sum * 1e-6 / 2.0 * 1000.0;
        }

        /// <summary>温度补偿流速（m/s），不依赖声程 L：v = c·(Tr'−Tf')/(Tr'+Tf')/cosθ。</summary>
        public static double VelocityMs(double soundSpeedMs, double forwardUs, double reverseUs, double angleDeg, double zeroNs, double delayUs)
        {
            if (!FlowMath.IsFinite(soundSpeedMs) || soundSpeedMs <= 0
                || !FlowMath.IsFinite(forwardUs) || !FlowMath.IsFinite(reverseUs)
                || !FlowMath.IsFinite(angleDeg) || !FlowMath.IsFinite(zeroNs) || !FlowMath.IsFinite(delayUs)
                || forwardUs <= 0 || reverseUs <= 0 || delayUs < 0
                || angleDeg < 0 || angleDeg >= 90)
            {
                return double.NaN;
            }

            double sumUs = (forwardUs - delayUs) + (reverseUs - delayUs);
            if (sumUs <= 0)
            {
                return double.NaN;
            }
            double deltaUs = (reverseUs - forwardUs) - zeroNs / 1000.0;
            return soundSpeedMs * deltaUs / sumUs / Math.Cos(angleDeg * Math.PI / 180.0);
        }
    }

    /// <summary>PT1000 两点线性标定：原始值 → °C。</summary>
    public static class TemperatureCalibration
    {
        public static bool IsUsable(double raw0, double raw1)
        {
            return FlowMath.IsFinite(raw0) && FlowMath.IsFinite(raw1) && Math.Abs(raw1 - raw0) >= 1.0;
        }

        public static double RawToCelsius(long raw, double raw0, double t0, double raw1, double t1)
        {
            if (!IsUsable(raw0, raw1) || !FlowMath.IsFinite(t0) || !FlowMath.IsFinite(t1))
            {
                return double.NaN;
            }
            return t0 + (raw - raw0) * (t1 - t0) / (raw1 - raw0);
        }
    }

    /// <summary>26 号工程 UserMeasure.c 里的默认两点标定（0°C / 100°C 计数）。</summary>
    public static class TemperatureDefaults
    {
        public const double Raw0 = 65097260.0;
        public const double Temp0C = 0.0;
        public const double Raw1 = 90163285.0;
        public const double Temp1C = 100.0;
    }

    /// <summary>
    /// 解析固件温度行的几种写法：
    ///   TEMPC,raw=73532188,uncalibrated                （26 号工程当前输出）
    ///   TEMPC,raw=73532188,t=33.6                      （固件已标定时）
    ///   TEMP,port=0,dir=0,ld=0,ul=0,stsF=2118,stsB=2119,resF=2124,resB=73534307
    ///   TEMP,25.3 / TEMP,25.3,8000 / TEMP_C=25.3       （直接温度）
    ///   TEMP,timeout / TEMPCODE,v1
    /// SCAN,/DUMP, 这类调试行忽略。
    /// </summary>
    public sealed class TemperatureParser
    {
        public bool TryParse(string rawLine, out TemperatureReading reading)
        {
            reading = null;
            if (rawLine == null)
            {
                return false;
            }
            string line = rawLine.Trim();
            if (line.Length == 0)
            {
                return false;
            }

            if (line.IndexOf("TEMPCODE", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                reading = new TemperatureReading();
                reading.Marker = true;
                return true;
            }

            int tempcIndex = line.IndexOf("TEMPC", StringComparison.OrdinalIgnoreCase);
            if (tempcIndex >= 0)
            {
                TemperatureReading result = new TemperatureReading();
                double rawValue;
                int rawIndex = line.IndexOf("raw=", tempcIndex, StringComparison.OrdinalIgnoreCase);
                if (rawIndex >= 0 && ReadNumberAfter(line, rawIndex + 4, out rawValue))
                {
                    result.HasRaw = true;
                    result.Raw = (long)Math.Round(rawValue);
                }
                double celsius;
                int tIndex = line.IndexOf(",t=", tempcIndex, StringComparison.OrdinalIgnoreCase);
                if (tIndex >= 0 && ReadNumberAfter(line, tIndex + 3, out celsius))
                {
                    result.HasValue = true;
                    result.ValueC = celsius;
                }
                if (result.HasRaw || result.HasValue)
                {
                    reading = result;
                    return true;
                }
                return false;
            }

            int cIndex = line.IndexOf("TEMP_C", StringComparison.OrdinalIgnoreCase);
            if (cIndex >= 0)
            {
                double direct;
                if (ReadNumberAfter(line, cIndex + 6, out direct))
                {
                    reading = new TemperatureReading();
                    reading.HasValue = true;
                    reading.ValueC = direct;
                    return true;
                }
                return false;
            }

            int index = line.IndexOf("TEMP", StringComparison.OrdinalIgnoreCase);
            if (index < 0)
            {
                return false;
            }
            int tail = index + 4;
            if (tail >= line.Length)
            {
                return false;
            }
            char separator = line[tail];
            if (separator != ',' && separator != '=' && separator != ' ' && separator != '\t')
            {
                return false;
            }

            string rest = line.Substring(tail + 1).Trim();
            if (rest.Length == 0)
            {
                return false;
            }

            TemperatureReading frame = new TemperatureReading();
            if (rest.StartsWith("timeout", StringComparison.OrdinalIgnoreCase))
            {
                frame.Timeout = true;
                reading = frame;
                return true;
            }

            // 新格式：TEMP,port=..,dir=..,stsF=..,stsB=..,resF=..,resB=..
            // 温度计数 = 结果 − 起始（dir=0 落在 B 通道，dir=1 落在 F 通道）
            if (rest.IndexOf("dir=", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                double stsB = double.NaN;
                double resB = double.NaN;
                double stsF = double.NaN;
                double resF = double.NaN;
                bool hasB = TryReadValueAfterTag(rest, "stsB", out stsB) && TryReadValueAfterTag(rest, "resB", out resB);
                bool hasF = TryReadValueAfterTag(rest, "stsF", out stsF) && TryReadValueAfterTag(rest, "resF", out resF);
                double dir;
                bool dirForward = TryReadValueAfterTag(rest, "dir", out dir) && Math.Abs(dir - 1.0) < 0.5;
                if ((dirForward && hasF) || (!dirForward && hasB))
                {
                    frame.HasRaw = true;
                    frame.Raw = (long)Math.Round(dirForward ? resF - stsF : resB - stsB);
                    reading = frame;
                    return true;
                }
                if (hasB)
                {
                    frame.HasRaw = true;
                    frame.Raw = (long)Math.Round(resB - stsB);
                    reading = frame;
                    return true;
                }
            }

            // 旧格式：TEMP,stsF=..,resF=.. （直接用 resF）
            double rawValue2;
            if (TryReadValueAfterTag(rest, "resF", out rawValue2))
            {
                frame.HasRaw = true;
                frame.Raw = (long)Math.Round(rawValue2);
                reading = frame;
                return true;
            }

            string[] parts = rest.Split(',');
            double celsiusValue;
            if (parts.Length > 0 && TryParseNumber(parts[0], out celsiusValue))
            {
                frame.HasValue = true;
                frame.ValueC = celsiusValue;
                if (parts.Length > 1)
                {
                    double raw3;
                    if (TryReadValueAfterTag(parts[1], "resF", out raw3) || TryParseNumber(parts[1], out raw3))
                    {
                        frame.HasRaw = true;
                        frame.Raw = (long)Math.Round(raw3);
                    }
                }
                reading = frame;
                return true;
            }

            return false;
        }

        private static bool TryReadValueAfterTag(string text, string tag, out double value)
        {
            value = double.NaN;
            int index = text.IndexOf(tag, StringComparison.OrdinalIgnoreCase);
            if (index < 0)
            {
                return false;
            }
            return ReadNumberAfter(text, index + tag.Length, out value);
        }

        private static bool ReadNumberAfter(string text, int start, out double value)
        {
            value = double.NaN;
            int index = start;
            while (index < text.Length && (text[index] == ' ' || text[index] == '\t'))
            {
                index++;
            }
            if (index < text.Length && (text[index] == '=' || text[index] == ','))
            {
                index++;
            }
            while (index < text.Length && (text[index] == ' ' || text[index] == '\t'))
            {
                index++;
            }

            int numberStart = index;
            if (index < text.Length && (text[index] == '+' || text[index] == '-'))
            {
                index++;
            }
            while (index < text.Length && (char.IsDigit(text[index]) || text[index] == '.'))
            {
                index++;
            }
            if (index == numberStart)
            {
                return false;
            }

            return double.TryParse(text.Substring(numberStart, index - numberStart),
                NumberStyles.Float, CultureInfo.InvariantCulture, out value);
        }

        private static bool TryParseNumber(string text, out double value)
        {
            value = double.NaN;
            if (text == null)
            {
                return false;
            }
            string trimmed = text.Trim();
            if (trimmed.Length == 0)
            {
                return false;
            }
            return double.TryParse(trimmed, NumberStyles.Float, CultureInfo.InvariantCulture, out value);
        }
    }

    /// <summary>把两点标定参数存到 exe 同目录的 temp_calib.ini，下次启动自动带上。</summary>
    public static class TemperatureCalibrationStore
    {
        private const string FileName = "temp_calib.ini";

        public static string[] Load()
        {
            try
            {
                string path = Path.Combine(AppContext.BaseDirectory, FileName);
                if (!File.Exists(path))
                {
                    return null;
                }

                string[] values = new string[4];
                string[] lines = File.ReadAllLines(path);
                for (int i = 0; i < lines.Length; i++)
                {
                    string line = lines[i].Trim();
                    if (line.Length == 0)
                    {
                        continue;
                    }
                    int equalIndex = line.IndexOf('=');
                    if (equalIndex <= 0)
                    {
                        continue;
                    }
                    string key = line.Substring(0, equalIndex).Trim();
                    string value = line.Substring(equalIndex + 1).Trim();
                    if (key == "raw0") values[0] = value;
                    else if (key == "t0") values[1] = value;
                    else if (key == "raw1") values[2] = value;
                    else if (key == "t1") values[3] = value;
                }

                if (values[0] == null || values[1] == null || values[2] == null || values[3] == null)
                {
                    return null;
                }
                return values;
            }
            catch (Exception)
            {
                return null;
            }
        }

        public static void Save(string raw0, string t0, string raw1, string t1)
        {
            try
            {
                string path = Path.Combine(AppContext.BaseDirectory, FileName);
                string[] lines = new string[]
                {
                    "raw0=" + raw0,
                    "t0=" + t0,
                    "raw1=" + raw1,
                    "t1=" + t1
                };
                File.WriteAllLines(path, lines);
            }
            catch (Exception)
            {
            }
        }
    }
}
