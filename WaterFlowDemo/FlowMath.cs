using System;

namespace WaterFlowDemo
{
    /// <summary>
    /// 时差法超声流量公式，已按 LS1D 问题跟踪表 #3 扣除固定延时 d：
    ///   Tf' = Tf − d, Tr' = Tr − d
    ///   v = L / (2·cosθ) · (1/Tf' − 1/Tr')
    /// 内部用 ΔT = (Tr − Tf) − 零点 再除以 Tf'·Tr'，数值更稳。
    /// </summary>
    public static class FlowMath
    {
        public static bool IsFinite(double value)
        {
            return !double.IsNaN(value) && !double.IsInfinity(value);
        }

        /// <summary>
        /// 计算流速（m/s）。
        /// </summary>
        /// <param name="forwardUs">顺流传播时间 Tf，微秒（芯片原始值）</param>
        /// <param name="reverseUs">逆流传播时间 Tr，微秒（芯片原始值）</param>
        /// <param name="lengthMm">有效声程 L，毫米</param>
        /// <param name="angleDeg">声路与水流夹角 θ，度（0 ≤ θ &lt; 90）</param>
        /// <param name="zeroNs">零点偏置，纳秒（静止水时测得的 ΔT）</param>
        /// <param name="delayUs">固定延时 d，微秒（电路 + 换能器延时，从两个飞行时间里扣掉）</param>
        public static double VelocityMs(double forwardUs, double reverseUs, double lengthMm, double angleDeg, double zeroNs, double delayUs)
        {
            if (!IsFinite(forwardUs) || !IsFinite(reverseUs) || !IsFinite(lengthMm) || !IsFinite(angleDeg)
                || !IsFinite(zeroNs) || !IsFinite(delayUs)
                || forwardUs <= 0 || reverseUs <= 0 || lengthMm <= 0 || angleDeg < 0 || angleDeg >= 90
                || delayUs < 0)
            {
                throw new ArgumentException("时间和声程须大于 0，夹角须在 0°（含）至 90°（不含）之间，固定延时不能为负。");
            }

            double forwardAcoustic = forwardUs - delayUs;
            double reverseAcoustic = reverseUs - delayUs;
            if (forwardAcoustic <= 0 || reverseAcoustic <= 0)
            {
                throw new ArgumentException("扣除固定延时后飞行时间必须大于 0，请检查 d 或 Tf/Tr。");
            }

            double deltaUs = (reverseUs - forwardUs) - zeroNs / 1000.0;

            // (ΔT / (Tf'·Tr')) 由 µs 换算成 1/s
            double result = lengthMm / 1000.0 / (2.0 * Math.Cos(angleDeg * Math.PI / 180.0))
                * (deltaUs * 1e-6) / ((forwardAcoustic * 1e-6) * (reverseAcoustic * 1e-6));

            if (!IsFinite(result))
            {
                throw new ArgumentException("输入数值超出可计算范围。");
            }
            return result;
        }

        /// <summary>
        /// 由流速和管内径计算瞬时流量（m³/h）。
        /// 与固件 SonarFlowCalc() 一致：A = π·D²/4 (mm²)，Q = v·A，再换算成 m³/h。
        /// </summary>
        public static double FlowM3h(double velocityMps, double pipeIdMm)
        {
            if (!IsFinite(velocityMps) || !IsFinite(pipeIdMm) || pipeIdMm <= 0)
            {
                return double.NaN;
            }
            double areaMm2 = Math.PI * pipeIdMm * pipeIdMm / 4.0;
            // v(m/s) × A(mm²) × 1e-6(m²/mm²) × 3600(s/h) = v·A·0.0036
            return velocityMps * areaMm2 * 0.0036;
        }

        /// <summary>
        /// 由几何尺寸反推等效声速 c = 2L / (Tf' + Tr')，用来检查声程/延时是否与芯片数据匹配。
        /// </summary>
        public static double SoundSpeedMs(double lengthMm, double forwardUs, double reverseUs, double delayUs)
        {
            if (!IsFinite(lengthMm) || !IsFinite(forwardUs) || !IsFinite(reverseUs) || !IsFinite(delayUs)
                || lengthMm <= 0 || forwardUs <= 0 || reverseUs <= 0 || delayUs < 0)
            {
                return double.NaN;
            }
            double sum = (forwardUs - delayUs) + (reverseUs - delayUs);
            if (sum <= 0)
            {
                return double.NaN;
            }
            return 2.0 * (lengthMm / 1000.0) / (sum * 1e-6);
        }
    }
}
