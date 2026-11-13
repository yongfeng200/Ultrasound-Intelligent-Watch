using System;
using System.Globalization;

namespace WaterFlowDemo
{
    public enum SonarParseResult
    {
        None = 0,
        Frame = 1,
        Duplicate = 2,
        Invalid = 3
    }

    public sealed class SonarFrame
    {
        public long Sequence { get; set; }
        public bool HasSequence { get; set; }
        public double ForwardPs { get; set; }
        public double BackwardPs { get; set; }
        public long Dtof { get; set; }
        public bool HasDtof { get; set; }
        public DateTime ReceivedAt { get; set; }
    }

    /// <summary>
    /// 解析 26.sonar_lcd_measure 的串口输出。支持两种格式：
    ///   SONAR_DATA,&lt;seq&gt;,&lt;dtof_ps&gt;,&lt;forward_ps&gt;,&lt;backward_ps&gt;
    ///   DTOF=123 , Mtime = 456 / tof_f =68508424 , tof_b =69668912   （旧固件逐行打印）
    /// 同时容错 SSCOM 加的时间戳前缀和行内空格。
    /// </summary>
    public sealed class SonarLineParser
    {
        private const int DuplicateWindowMs = 150;   // 同一帧被两种格式重复打印的判重窗口
        private const int SonarDataHoldMs = 1200;    // 出现过 SONAR_DATA 后，忽略旧格式，避免重复计数

        private DateTime _lastEmit = DateTime.MinValue;
        private DateTime _lastSonarData = DateTime.MinValue;
        private double _lastForward = double.NaN;
        private double _lastBackward = double.NaN;
        private double _pendingForward = double.NaN;
        private double _pendingBackward = double.NaN;

        public SonarParseResult TryParse(string rawLine, out SonarFrame frame)
        {
            frame = null;
            if (rawLine == null)
            {
                return SonarParseResult.None;
            }

            string line = rawLine.Trim();
            if (line.Length == 0)
            {
                return SonarParseResult.None;
            }

            string payload = CutToTag(line);
            if (payload == null)
            {
                return SonarParseResult.None;
            }

            if (payload.StartsWith("SONAR_DATA", StringComparison.OrdinalIgnoreCase))
            {
                return ParseSonarData(payload, out frame);
            }
            if (payload.StartsWith("tof_f", StringComparison.OrdinalIgnoreCase)
                || payload.StartsWith("tof_b", StringComparison.OrdinalIgnoreCase))
            {
                return ParseTofLine(payload, out frame);
            }
            return SonarParseResult.None;
        }

        private SonarParseResult ParseSonarData(string payload, out SonarFrame frame)
        {
            frame = null;
            string[] parts = payload.Split(',');
            if (parts.Length < 5)
            {
                return SonarParseResult.Invalid;
            }

            long sequence;
            long dtof;
            double forward;
            double backward;

            if (!long.TryParse(parts[1].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out sequence)
                || !long.TryParse(parts[2].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out dtof)
                || !double.TryParse(parts[3].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out forward)
                || !double.TryParse(parts[4].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out backward))
            {
                return SonarParseResult.Invalid;
            }

            if (!IsPlausible(forward, backward))
            {
                return SonarParseResult.Invalid;
            }

            _lastSonarData = DateTime.Now;
            _pendingForward = double.NaN;
            _pendingBackward = double.NaN;

            SonarParseResult parsed = BuildFrame(sequence, true, forward, backward, out frame);
            if (frame != null)
            {
                frame.Dtof = dtof;
                frame.HasDtof = true;
            }
            return parsed;
        }

        private SonarParseResult ParseTofLine(string payload, out SonarFrame frame)
        {
            frame = null;

            double parsedForward = ExtractNumber(payload, "tof_f");
            double parsedBackward = ExtractNumber(payload, "tof_b");
            if (!double.IsNaN(parsedForward))
            {
                _pendingForward = parsedForward;
            }
            if (!double.IsNaN(parsedBackward))
            {
                _pendingBackward = parsedBackward;
            }

            if (double.IsNaN(_pendingForward) || double.IsNaN(_pendingBackward))
            {
                return SonarParseResult.None;
            }
            if (!IsPlausible(_pendingForward, _pendingBackward))
            {
                _pendingForward = double.NaN;
                _pendingBackward = double.NaN;
                return SonarParseResult.Invalid;
            }

            // 新固件同时打印 SONAR_DATA，忽略旧格式这几行，避免一帧算两次
            if ((DateTime.Now - _lastSonarData).TotalMilliseconds < SonarDataHoldMs)
            {
                return SonarParseResult.None;
            }

            double forward = _pendingForward;
            double backward = _pendingBackward;
            _pendingForward = double.NaN;
            _pendingBackward = double.NaN;
            return BuildFrame(0, false, forward, backward, out frame);
        }

        private SonarParseResult BuildFrame(long sequence, bool hasSequence, double forward, double backward, out SonarFrame frame)
        {
            frame = null;
            DateTime now = DateTime.Now;

            if (forward == _lastForward && backward == _lastBackward
                && (now - _lastEmit).TotalMilliseconds < DuplicateWindowMs)
            {
                return SonarParseResult.Duplicate;
            }

            _lastEmit = now;
            _lastForward = forward;
            _lastBackward = backward;

            frame = new SonarFrame();
            frame.Sequence = sequence;
            frame.HasSequence = hasSequence;
            frame.ForwardPs = forward;
            frame.BackwardPs = backward;
            frame.ReceivedAt = now;
            return SonarParseResult.Frame;
        }

        private static bool IsPlausible(double forwardPs, double backwardPs)
        {
            // 68µs 量级 → 6.8e7 ps；上限放到 1e9 ps(1ms)，下限 1e4 ps(10ns)
            return FlowMath.IsFinite(forwardPs) && FlowMath.IsFinite(backwardPs)
                && forwardPs >= 1e4 && forwardPs <= 1e9
                && backwardPs >= 1e4 && backwardPs <= 1e9;
        }

        /// <summary>
        /// 截断 SSCOM 的时间戳前缀（[10:44:07.653]收←◆）等噪声，只保留从标签开始的内容。
        /// </summary>
        private static string CutToTag(string line)
        {
            string[] tags = new string[] { "SONAR_DATA", "tof_f", "tof_b", "FLOW_DATA" };
            for (int i = 0; i < tags.Length; i++)
            {
                int index = line.IndexOf(tags[i], StringComparison.OrdinalIgnoreCase);
                if (index >= 0)
                {
                    string payload = line.Substring(index);
                    return payload.StartsWith("FLOW_DATA", StringComparison.OrdinalIgnoreCase) ? null : payload;
                }
            }
            return null;
        }

        private static double ExtractNumber(string text, string tag)
        {
            int tagIndex = text.IndexOf(tag, StringComparison.OrdinalIgnoreCase);
            if (tagIndex < 0)
            {
                return double.NaN;
            }
            int equalIndex = text.IndexOf('=', tagIndex + tag.Length);
            if (equalIndex < 0)
            {
                return double.NaN;
            }

            int index = equalIndex + 1;
            while (index < text.Length && (text[index] == ' ' || text[index] == '\t'))
            {
                index++;
            }

            int start = index;
            if (index < text.Length && (text[index] == '+' || text[index] == '-'))
            {
                index++;
            }
            while (index < text.Length && char.IsDigit(text[index]))
            {
                index++;
            }
            if (index == start)
            {
                return double.NaN;
            }

            double value;
            if (!double.TryParse(text.Substring(start, index - start), NumberStyles.Integer, CultureInfo.InvariantCulture, out value))
            {
                return double.NaN;
            }
            return value;
        }
    }
}
