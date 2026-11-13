using System;
using System.Globalization;
using System.IO;
using System.Text;

namespace WaterFlowDemo
{
    /// <summary>
    /// 无硬件自检：验证串口行解析、去重、公式和整段样例日志。
    /// 运行 WaterFlowDemo.exe --selftest，结果写入 selftest.log，退出码 0 表示全部通过。
    /// </summary>
    public static class SelfTest
    {
        private static int _pass;
        private static int _fail;
        private static StringBuilder _log;

        public static int Run()
        {
            _pass = 0;
            _fail = 0;
            _log = new StringBuilder();
            _log.AppendLine("WaterFlowDemo 自检  " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture));
            _log.AppendLine("------------------------------------------------------------");

            ParserTests();
            MathTests();
            TemperatureTests();
            SampleSequenceTest();

            _log.AppendLine("------------------------------------------------------------");
            _log.AppendLine(string.Format(CultureInfo.InvariantCulture, "结果：{0} 通过 / {1} 失败", _pass, _fail));

            try
            {
                File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "selftest.log"), _log.ToString(), new UTF8Encoding(false));
            }
            catch (Exception)
            {
            }
            return _fail == 0 ? 0 : 1;
        }

        private static void Check(string name, bool passed)
        {
            if (passed)
            {
                _pass++;
                _log.AppendLine("[PASS] " + name);
            }
            else
            {
                _fail++;
                _log.AppendLine("[FAIL] " + name);
            }
        }

        private static void ParserTests()
        {
            SonarLineParser parser = new SonarLineParser();
            SonarFrame frame;

            SonarParseResult result = parser.TryParse("SONAR_DATA,805, 864087, 68508424, 69668912", out frame);
            Check("SONAR_DATA：逗号后带空格也能解析",
                result == SonarParseResult.Frame && frame.Sequence == 805
                && frame.ForwardPs == 68508424 && frame.BackwardPs == 69668912);

            result = parser.TryParse("[10:44:08.384]收←◆SONAR_DATA,807, -625632, 69532240, 68690552", out frame);
            Check("SSCOM 时间戳前缀被剥离",
                result == SonarParseResult.Frame && frame.Sequence == 807 && frame.BackwardPs == 68690552);

            SonarLineParser legacy = new SonarLineParser();
            result = legacy.TryParse("DTOF=906061 , Mtime = 864087", out frame);
            Check("只有 DTOF 行时不急于出帧", result == SonarParseResult.None);
            result = legacy.TryParse("tof_f =68508424 , tof_b =69668912", out frame);
            Check("旧格式 tof_f/tof_b 回退解析",
                result == SonarParseResult.Frame && frame.ForwardPs == 68508424 && frame.BackwardPs == 69668912);

            SonarLineParser dedupe = new SonarLineParser();
            dedupe.TryParse("SONAR_DATA,1, 100, 1000000, 1001000", out frame);
            result = dedupe.TryParse("tof_f =1000000 , tof_b =1001000", out frame);
            Check("同一帧两种格式只计一次", result == SonarParseResult.Duplicate || result == SonarParseResult.None);

            result = new SonarLineParser().TryParse("SONAR_DATA,2,0,0,0", out frame);
            Check("全 0 数据判为无效", result == SonarParseResult.Invalid);

            result = new SonarLineParser().TryParse("PTR=8103000c T0=1 hitF=12 hitB=0 stF=3 stB=129", out frame);
            Check("调试噪声行被忽略", result == SonarParseResult.None);
        }

        private static void MathTests()
        {
            double velocity = FlowMath.VelocityMs(27.001, 27.053, 40, 45, 0, 0);
            Check(string.Format(CultureInfo.InvariantCulture, "demo 示例 Tf=27.001/Tr=27.053/L=40/θ=45 → {0:0.000000} m/s", velocity),
                Math.Abs(velocity - 2.0135) < 0.001);

            double zeroed = FlowMath.VelocityMs(27.001, 27.053, 40, 45, 52.0, 0);
            Check("扣掉 52ns 零点后流速为 0", Math.Abs(zeroed) < 1e-6);

            double reversed = FlowMath.VelocityMs(27.053, 27.001, 40, 45, 0, 0);
            Check("逆流时流速为负", reversed < -2.0);

            double soundSpeed = FlowMath.SoundSpeedMs(40, 27.026, 27.026, 0);
            Check(string.Format(CultureInfo.InvariantCulture, "等效声速 {0:0.0} m/s ≈ 1480", soundSpeed),
                Math.Abs(soundSpeed - 1480.0) < 2.0);

            double withDelay = FlowMath.VelocityMs(68.601120, 68.727616, 62, 45, 0, 27.5);
            Check(string.Format(CultureInfo.InvariantCulture, "扣除 d=27.5µs 后流速 {0:0.000} m/s", withDelay),
                Math.Abs(withDelay - 3.273) < 0.02);

            double cWithDelay = FlowMath.SoundSpeedMs(62, 68.601120, 68.727616, 27.5);
            Check(string.Format(CultureInfo.InvariantCulture, "扣除 d 后实测声速 {0:0.0} m/s", cWithDelay),
                Math.Abs(cWithDelay - 1506.2) < 1.0);

            double q15 = FlowMath.FlowM3h(3.286, 15);
            Check(string.Format(CultureInfo.InvariantCulture, "DN15 流量 {0:0.000} m³/h", q15),
                Math.Abs(q15 - 2.090) < 0.01);

            double q12 = FlowMath.FlowM3h(3.286, 12);
            Check(string.Format(CultureInfo.InvariantCulture, "DN12 流量 {0:0.000} m³/h", q12),
                Math.Abs(q12 - 1.338) < 0.01);

            double q194 = FlowMath.FlowM3h(3.286, 19.4);
            Check(string.Format(CultureInfo.InvariantCulture, "DN19.4 流量 {0:0.000} m³/h", q194),
                Math.Abs(q194 - 3.497) < 0.01);

            bool threw = false;
            try
            {
                FlowMath.VelocityMs(0, 27.053, 40, 45, 0, 0);
            }
            catch (ArgumentException)
            {
                threw = true;
            }
            Check("时间为 0 时抛出参数异常", threw);

            threw = false;
            try
            {
                FlowMath.VelocityMs(27.001, 27.053, 40, 90, 0, 0);
            }
            catch (ArgumentException)
            {
                threw = true;
            }
            Check("夹角 90° 被拒绝", threw);
        }

        private static void TemperatureTests()
        {
            TemperatureParser parser = new TemperatureParser();
            TemperatureReading reading;

            bool ok = parser.TryParse("TEMP,stsF=1204,resF=8000,stsB=1205,resB=8002", out reading);
            Check("TEMP 旧格式回退（resF）",
                ok && reading.HasRaw && reading.Raw == 8000 && !reading.HasValue);

            ok = new TemperatureParser().TryParse("TEMPC,raw=73532188,uncalibrated", out reading);
            Check("TEMPC 原始值行",
                ok && reading.HasRaw && reading.Raw == 73532188 && !reading.HasValue);

            ok = new TemperatureParser().TryParse("TEMPC,raw=73532188,t=33.6", out reading);
            Check("TEMPC 带固件温度",
                ok && reading.HasRaw && reading.HasValue && Math.Abs(reading.ValueC - 33.6) < 1e-9);

            ok = new TemperatureParser().TryParse("TEMP,port=0,dir=0,ld=0,ul=0,stsF=2118,stsB=2119,resF=2124,resB=73534307", out reading);
            Check("TEMP 新格式取 resB−stsB",
                ok && reading.HasRaw && reading.Raw == 73532188);

            ok = new TemperatureParser().TryParse("TEMP,25.3", out reading);
            Check("TEMP,25.3 直接温度", ok && reading.HasValue && Math.Abs(reading.ValueC - 25.3) < 1e-9);

            ok = new TemperatureParser().TryParse("TEMP_C=26.1", out reading);
            Check("TEMP_C=26.1 直接温度", ok && reading.HasValue && Math.Abs(reading.ValueC - 26.1) < 1e-9);

            ok = new TemperatureParser().TryParse("TEMP,timeout", out reading);
            Check("TEMP,timeout 识别", ok && reading.Timeout);

            ok = new TemperatureParser().TryParse("TEMPCODE,v1", out reading);
            Check("TEMPCODE 标记", ok && reading.Marker);

            ok = new TemperatureParser().TryParse("SCAN,ld=0,ul=0,stsF=1,resF=2", out reading);
            Check("SCAN 调试行忽略", !ok);

            double celsius = TemperatureCalibration.RawToCelsius(8000, 0, 0, 10000, 25);
            Check(string.Format(CultureInfo.InvariantCulture, "两点标定 8000 → {0:0.0}°C", celsius),
                Math.Abs(celsius - 20.0) < 1e-9);

            double defaults = TemperatureCalibration.RawToCelsius(73532188,
                TemperatureDefaults.Raw0, TemperatureDefaults.Temp0C,
                TemperatureDefaults.Raw1, TemperatureDefaults.Temp1C);
            Check(string.Format(CultureInfo.InvariantCulture, "固件默认标定 73532188 → {0:0.00}°C", defaults),
                Math.Abs(defaults - 33.65) < 0.05);

            Check("两点原始值相同时转换无效",
                double.IsNaN(TemperatureCalibration.RawToCelsius(8000, 5, 0, 5, 25)));

            double c0 = WaterTemperature.SoundSpeedMs(0);
            double c20 = WaterTemperature.SoundSpeedMs(20);
            double c25 = WaterTemperature.SoundSpeedMs(25);
            Check(string.Format(CultureInfo.InvariantCulture, "0°C 声速 {0:0.0} m/s", c0), Math.Abs(c0 - 1402.4) < 1.0);
            Check(string.Format(CultureInfo.InvariantCulture, "20°C 声速 {0:0.0} m/s", c20), Math.Abs(c20 - 1482.4) < 1.0);
            Check(string.Format(CultureInfo.InvariantCulture, "25°C 声速 {0:0.0} m/s", c25), Math.Abs(c25 - 1496.7) < 1.0);

            double path = WaterTemperature.PathMm(c20, 68.601120, 68.727616, 0);
            Check(string.Format(CultureInfo.InvariantCulture, "20°C 反推声程 {0:0.0} mm", path), Math.Abs(path - 101.8) < 0.5);

            double velocity = WaterTemperature.VelocityMs(c20, 68.601120, 68.727616, 45, 0, 0);
            Check(string.Format(CultureInfo.InvariantCulture, "温度法流速 {0:0.000} m/s", velocity), Math.Abs(velocity - 1.93) < 0.03);

            double pathDelay = WaterTemperature.PathMm(1506.16, 68.601120, 68.727616, 27.5);
            Check(string.Format(CultureInfo.InvariantCulture, "反推声程（扣 d）{0:0.00} mm", pathDelay),
                Math.Abs(pathDelay - 62.0) < 0.3);

            double vTempDelay = WaterTemperature.VelocityMs(1506.16, 68.601120, 68.727616, 45, 0, 27.5);
            Check(string.Format(CultureInfo.InvariantCulture, "温度法（扣 d）{0:0.000} m/s", vTempDelay),
                Math.Abs(vTempDelay - 3.273) < 0.02);
        }

        private static void SampleSequenceTest()
        {
            string[] lines = new string[]
            {
                "[10:44:07.653]收←◆",
                "DTOF=906061 , Mtime = 864087",
                "tof_f =68508424 , tof_b =69668912",
                "SONAR_DATA,805, 864087, 68508424, 69668912",
                "[10:44:07.914]收←◆",
                "DTOF=124440 , Mtime = 118675",
                "tof_f =69497912 , tof_b =69657760",
                "SONAR_DATA,806, 118675, 69497912, 69657760",
                "[10:44:08.123]收←◆",
                "DTOF=-656023 , Mtime = -625632",
                "tof_f =69532240 , tof_b =68690552",
                "SONAR_DATA,807, -625632, 69532240, 68690552",
                "[10:44:08.384]收←◆timeout 8103000c",
                "PTR=8103000c T0=1 hitF=12 hitB=0 stF=3 stB=129",
                "[10:44:08.639]收←◆",
                "DTOF=116058 , Mtime = 110682",
                "tof_f =69502088 , tof_b =69464724",
                "SONAR_DATA,806, 110682, 69502088, 69464724"
            };

            SonarLineParser parser = new SonarLineParser();
            int frames = 0;
            SonarFrame last = null;
            SonarFrame frame;
            for (int i = 0; i < lines.Length; i++)
            {
                if (parser.TryParse(lines[i], out frame) == SonarParseResult.Frame)
                {
                    frames++;
                    last = frame;
                }
            }

            Check(string.Format(CultureInfo.InvariantCulture, "整段样例日志解析出 4 帧（实际 {0}）", frames), frames == 4);
            Check("最后一帧取到 69464724 ps", last != null && last.BackwardPs == 69464724);

            if (last != null)
            {
                double velocity = FlowMath.VelocityMs(last.ForwardPs / 1e6, last.BackwardPs / 1e6, 40, 45, 0, 0);
                _log.AppendLine(string.Format(CultureInfo.InvariantCulture,
                    "       最后一帧 Tf={0:0.000000}μs Tr={1:0.000000}μs → v={2:0.000} m/s（L=40mm,θ=45°）",
                    last.ForwardPs / 1e6, last.BackwardPs / 1e6, velocity));
            }
        }
    }
}
