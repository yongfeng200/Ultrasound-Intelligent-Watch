using System;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace WaterFlowDemo;
public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        Calculate();
        if (Array.IndexOf(Environment.GetCommandLineArgs(), "--screenshot") >= 0)
            Loaded += (_, _) => Dispatcher.BeginInvoke(DispatcherPriority.ApplicationIdle, new Action(() =>
            {
                var surface = (FrameworkElement)Content;
                var bitmap = new RenderTargetBitmap((int)surface.ActualWidth, (int)surface.ActualHeight, 96, 96, PixelFormats.Pbgra32);
                bitmap.Render(surface);
                var encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create(bitmap));
                using (var file = File.Create(Path.Combine(AppContext.BaseDirectory, "preview.png"))) encoder.Save(file);
                Close();
            }));
    }

    public static double FlowVelocity(double forwardUs, double reverseUs, double lengthMm, double angleDeg)
    {
        if (!double.IsFinite(forwardUs) || !double.IsFinite(reverseUs) || !double.IsFinite(lengthMm) || !double.IsFinite(angleDeg)
            || forwardUs <= 0 || reverseUs <= 0 || lengthMm <= 0 || angleDeg < 0 || angleDeg >= 90)
            throw new ArgumentException("时间和声程须大于 0，夹角须在 0°（含）至 90°（不含）之间。");
        // Input: microseconds and millimeters. Convert to SI before applying the reciprocal transit-time formula.
        double result = lengthMm / 1000 / (2 * Math.Cos(angleDeg * Math.PI / 180))
            * (1 / (forwardUs * 1e-6) - 1 / (reverseUs * 1e-6));
        if (!double.IsFinite(result)) throw new ArgumentException("输入数值超出可计算范围。");
        return result;
    }
    private void Calculate()
    {
        try
        {
            double tf = Read(ForwardInput.Text), tr = Read(ReverseInput.Text), length = Read(LengthInput.Text), angle = Read(AngleInput.Text);
            double speed = FlowVelocity(tf, tr, length, angle);
            ForwardValue.Text = tf.ToString("0.000###", CultureInfo.InvariantCulture);
            ReverseValue.Text = tr.ToString("0.000###", CultureInfo.InvariantCulture);
            VelocityValue.Text = speed.ToString("0.000", CultureInfo.InvariantCulture);
            DeltaValue.Text = ((tr - tf) * 1000).ToString("0.00", CultureInfo.InvariantCulture);
            DirectionLabel.Text = speed > 0 ? "→  正向流动" : speed < 0 ? "←  反向流动" : "—  零流速";
            GeometryLabel.Text = string.Format("声程 {0:g} mm   /   夹角 {1:g}°", length, angle);
            StatusLabel.Foreground = new SolidColorBrush(Color.FromRgb(115, 133, 155));
            StatusLabel.Text = "演示值已计算 · 时间单位 μs，流速单位 m/s";
        }
        catch (Exception ex) when (ex is FormatException || ex is ArgumentException || ex is OverflowException)
        {
            ForwardValue.Text = ReverseValue.Text = VelocityValue.Text = DeltaValue.Text = "—";
            DirectionLabel.Text = "请检查输入";
            GeometryLabel.Text = "等待有效参数";
            StatusLabel.Foreground = Brushes.Firebrick;
            StatusLabel.Text = ex is FormatException ? "请输入有效数字，小数点使用“.”。" : ex.Message;
        }
    }
    private static double Read(string value) => double.Parse(value, NumberStyles.Float, CultureInfo.InvariantCulture);
    private void Calculate_Click(object sender, RoutedEventArgs e) => Calculate();
    private void Reset_Click(object sender, RoutedEventArgs e)
    {
        ForwardInput.Text = "27.001"; ReverseInput.Text = "27.053";
        LengthInput.Text = "40"; AngleInput.Text = "45";
        Calculate();
    }
}
