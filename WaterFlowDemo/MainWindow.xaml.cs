using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Microsoft.Win32;

namespace WaterFlowDemo
{
    public partial class MainWindow : Window
    {
        private const int RawLineLimit = 200;
        private const string RecordHeader = "时间,序号,Tf(us),Tr(us),dT(ns),v(m/s),水温(C),温度raw,DTOF(ps),实测声速(m/s),理论声速(m/s),L(mm),角度(deg),d(us),内径(mm),流量(m3/h),零点(ns)";
        private static readonly CultureInfo Invariant = CultureInfo.InvariantCulture;

        private sealed class MeasureRecord
        {
            public DateTime Time;
            public string Line;
        }

        private readonly List<MeasureRecord> _records = new List<MeasureRecord>();
        private StreamWriter _recordWriter;
        private string _recordPath;
        private DateTime _recordStartedAt = DateTime.MinValue;

        private static readonly Brush AccentBrush = MakeBrush(0x18, 0x78, 0xF2);
        private static readonly Brush DangerBrush = MakeBrush(0xE0, 0x53, 0x4B);
        private static readonly Brush DotIdle = MakeBrush(0xC9, 0xD4, 0xE1);
        private static readonly Brush DotOk = MakeBrush(0x1B, 0x8A, 0x5A);
        private static readonly Brush DotBad = MakeBrush(0xE0, 0x53, 0x4B);
        private static readonly Brush TextMuted = MakeBrush(0x73, 0x85, 0x9B);
        private static readonly Brush TextBad = MakeBrush(0xD3, 0x3E, 0x34);
        private static readonly Brush TextGreen = MakeBrush(0x1B, 0x8A, 0x5A);
        private static readonly Brush TextAmber = MakeBrush(0xA8, 0x62, 0x00);
        private static readonly Brush BadgeDemoBg = MakeBrush(0xE7, 0xEF, 0xFC);
        private static readonly Brush BadgeDemoFg = MakeBrush(0x39, 0x75, 0xBF);
        private static readonly Brush BadgeLiveBg = MakeBrush(0xE4, 0xF6, 0xEE);
        private static readonly Brush BadgeLiveFg = MakeBrush(0x1B, 0x8A, 0x5A);
        private static readonly Brush BadgeReplayBg = MakeBrush(0xFF, 0xF3, 0xE0);
        private static readonly Brush BadgeReplayFg = MakeBrush(0xB2, 0x5E, 0x09);
        private static readonly Brush BadgeNeutralBg = MakeBrush(0xED, 0xF2, 0xF8);
        private static readonly Brush BadgeNeutralFg = MakeBrush(0x50, 0x67, 0x83);
        private static readonly Brush NavActiveBrush = MakeBrush(0x24, 0x3F, 0x60);
        private static readonly Brush NavIdleBrush = MakeBrush(0x82, 0x9B, 0xB5);

        private SonarLineParser _parser = new SonarLineParser();
        private readonly TemperatureParser _tempParser = new TemperatureParser();
        private readonly object _frameLock = new object();
        private readonly List<string> _rawLines = new List<string>();
        private readonly DispatcherTimer _uiTimer;

        private SerialLink _serial;
        private SonarFrame _pendingFrame;
        private int _frameCount;
        private int _invalidCount;
        private int _lineCount;
        private DateTime _lastFrameAt = DateTime.MinValue;
        private bool _statusLocked;

        private long? _lastRawTemp;
        private DateTime _lastRawTempAt = DateTime.MinValue;
        private bool _rawFromSerial;
        private double _firmwareTempC = double.NaN;
        private double _currentTempC = double.NaN;
        private bool _tempModuleSeen;
        private DateTime _tempTimeoutAt = DateTime.MinValue;
        private bool _initialized;
        private bool _updatingTempFields;

        private bool _replayMode;
        private string _replayPath;
        private string _autoPort;
        private int _autoBaud;
        private bool _screenshot;
        private int _startPage;
        private bool _autoRecord;
        private DispatcherTimer _replayTimer;

        public MainWindow()
        {
            InitializeComponent();

            _uiTimer = new DispatcherTimer();
            _uiTimer.Interval = TimeSpan.FromMilliseconds(120);
            _uiTimer.Tick += UiTimer_Tick;
            _uiTimer.Start();

            InitializeBaudRates();
            LoadTemperatureCalibration();
            ParseCommandLine();

            _initialized = true;
            RefreshTemperature();
            ShowPage(_startPage);
            Calculate();
            if (_autoRecord)
            {
                StartRecording();
            }
            SetBadge(BadgeDemoBg, BadgeDemoFg, "●  演示数据 / 未连接串口");

            Loaded += MainWindow_Loaded;
            Closing += MainWindow_Closing;
        }

        private static Brush MakeBrush(byte red, byte green, byte blue)
        {
            SolidColorBrush brush = new SolidColorBrush(Color.FromRgb(red, green, blue));
            brush.Freeze();
            return brush;
        }

        private void InitializeBaudRates()
        {
            string[] rates = new string[] { "9600", "19200", "38400", "57600", "115200", "230400", "460800" };
            for (int i = 0; i < rates.Length; i++)
            {
                BaudCombo.Items.Add(rates[i]);
            }
            BaudCombo.SelectedItem = "115200";
        }

        private void ParseCommandLine()
        {
            string[] args = Environment.GetCommandLineArgs();

            int replayIndex = Array.IndexOf(args, "--replay");
            if (replayIndex >= 0 && replayIndex + 1 < args.Length)
            {
                _replayPath = args[replayIndex + 1];
            }
            int portIndex = Array.IndexOf(args, "--port");
            if (portIndex >= 0 && portIndex + 1 < args.Length)
            {
                _autoPort = args[portIndex + 1];
            }
            int baudIndex = Array.IndexOf(args, "--baud");
            if (baudIndex >= 0 && baudIndex + 1 < args.Length)
            {
                int.TryParse(args[baudIndex + 1], NumberStyles.Integer, Invariant, out _autoBaud);
            }

            string length;
            if (TryGetArgument(args, "--length", out length))
            {
                LengthInput.Text = length;
            }
            string angle;
            if (TryGetArgument(args, "--angle", out angle))
            {
                AngleInput.Text = angle;
            }
            string zero;
            if (TryGetArgument(args, "--zero", out zero))
            {
                ZeroInput.Text = zero;
            }
            string delay;
            if (TryGetArgument(args, "--delay", out delay))
            {
                DelayInput.Text = delay;
            }
            string pipe;
            if (TryGetArgument(args, "--pipe", out pipe))
            {
                PipeIdInput.Text = pipe;
            }
            string raw;
            if (TryGetArgument(args, "--raw", out raw))
            {
                RawInput.Text = raw;
                _rawFromSerial = false;
            }
            string page;
            if (TryGetArgument(args, "--page", out page))
            {
                int.TryParse(page, NumberStyles.Integer, Invariant, out _startPage);
            }

            _screenshot = Array.IndexOf(args, "--screenshot") >= 0;
            _autoRecord = Array.IndexOf(args, "--record") >= 0;
        }

        private static bool TryGetArgument(string[] args, string name, out string value)
        {
            value = null;
            int index = Array.IndexOf(args, name);
            if (index < 0 || index + 1 >= args.Length)
            {
                return false;
            }
            value = args[index + 1];
            return true;
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            RefreshPorts();

            if (!string.IsNullOrEmpty(_replayPath))
            {
                StartReplay(_replayPath);
            }
            else if (!string.IsNullOrEmpty(_autoPort))
            {
                OpenSerial(_autoPort, _autoBaud > 0 ? _autoBaud : 115200);
            }
            if (_screenshot)
            {
                int delay;
                if (!string.IsNullOrEmpty(_replayPath))
                {
                    delay = 4500;
                }
                else if (!string.IsNullOrEmpty(_autoPort))
                {
                    delay = 3500;
                }
                else
                {
                    delay = 1100;
                }
                ScheduleScreenshot(delay);
            }
        }

        private void MainWindow_Closing(object sender, CancelEventArgs e)
        {
            SaveTemperatureCalibration();
            CloseRecordWriter();
            if (_uiTimer != null)
            {
                _uiTimer.Stop();
            }
            if (_replayTimer != null)
            {
                _replayTimer.Stop();
            }
            DisconnectSerial(null);
        }

        // ------------------------------------------------------------------
        // 页面切换
        // ------------------------------------------------------------------
        private void Nav_Click(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            if (button == null)
            {
                return;
            }
            int page;
            if (!int.TryParse(Convert.ToString(button.Tag, Invariant), out page))
            {
                return;
            }
            ShowPage(page);
        }

        private void ShowPage(int page)
        {
            PageMonitor.Visibility = page == 0 ? Visibility.Visible : Visibility.Collapsed;
            PageTemp.Visibility = page == 1 ? Visibility.Visible : Visibility.Collapsed;
            PageRaw.Visibility = page == 2 ? Visibility.Visible : Visibility.Collapsed;
            PageRecord.Visibility = page == 3 ? Visibility.Visible : Visibility.Collapsed;

            SetNav(NavMonitor, page == 0);
            SetNav(NavTemp, page == 1);
            SetNav(NavRaw, page == 2);
            SetNav(NavRecord, page == 3);

            if (page == 1)
            {
                PageTitle.Text = "温度转换";
                PageSubtitle.Text = "PT1000 原始计数 → 摄氏温度 · 声速与声程";
            }
            else if (page == 2)
            {
                PageTitle.Text = "原始数据";
                PageSubtitle.Text = "串口收到的全部行，便于核对协议";
            }
            else if (page == 3)
            {
                PageTitle.Text = "记录导出";
                PageSubtitle.Text = "测量数据写入 CSV · Excel / 记事本都能直接打开";
            }
            else
            {
                PageTitle.Text = "实时监测";
                PageSubtitle.Text = "顺逆流传播时间 · 流速与水温";
            }
        }

        private void SetNav(Button button, bool active)
        {
            button.Background = active ? NavActiveBrush : Brushes.Transparent;
            button.Foreground = active ? Brushes.White : NavIdleBrush;
        }

        private void TempCard_Click(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            ShowPage(1);
        }

        // ------------------------------------------------------------------
        // 串口
        // ------------------------------------------------------------------
        private void Refresh_Click(object sender, RoutedEventArgs e)
        {
            RefreshPorts();
        }

        private void RefreshPorts()
        {
            if (_serial != null && _serial.IsOpen)
            {
                return;
            }
            if (!_replayMode)
            {
                SetSerialStatus("正在扫描串口…", false);
            }

            Task<List<PortInfo>> task = Task.Factory.StartNew<List<PortInfo>>(SerialPorts.GetPorts);
            task.ContinueWith(delegate(Task<List<PortInfo>> finished)
            {
                List<PortInfo> ports = finished.Status == TaskStatus.RanToCompletion
                    ? finished.Result
                    : new List<PortInfo>();
                try
                {
                    Dispatcher.BeginInvoke(new Action<List<PortInfo>>(ApplyPorts), ports);
                }
                catch (Exception)
                {
                }
            });
        }

        private void ApplyPorts(List<PortInfo> ports)
        {
            string previousName = null;
            PortInfo previous = PortCombo.SelectedItem as PortInfo;
            if (previous != null)
            {
                previousName = previous.Name;
            }

            PortCombo.Items.Clear();
            PortInfo selected = null;
            for (int i = 0; i < ports.Count; i++)
            {
                PortCombo.Items.Add(ports[i]);
                if (previousName != null && ports[i].Name == previousName)
                {
                    selected = ports[i];
                }
            }
            if (selected == null && ports.Count > 0)
            {
                selected = ports[0];
            }
            if (ports.Count == 0)
            {
                PortInfo placeholder = new PortInfo();
                placeholder.Name = "";
                placeholder.Description = "未发现串口 · 请检查驱动";
                PortCombo.Items.Add(placeholder);
                selected = placeholder;
            }
            PortCombo.SelectedItem = selected;

            if (_serial != null && _serial.IsOpen)
            {
                return;
            }
            if (_replayMode)
            {
                return;
            }
            if (ports.Count == 0)
            {
                SetSerialStatus("未发现串口 · 请检查 USB 线与 CH340 驱动", true);
            }
            else if (!_statusLocked)
            {
                SetSerialStatus(string.Format(Invariant, "未连接串口 · 发现 {0} 个串口", ports.Count), false);
            }
        }

        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            if (_replayMode)
            {
                SetSerialStatus("日志回放模式，未使用串口", false);
                return;
            }
            if (_serial != null && _serial.IsOpen)
            {
                DisconnectSerial(null);
                return;
            }

            PortInfo port = PortCombo.SelectedItem as PortInfo;
            if (port == null || string.IsNullOrEmpty(port.Name))
            {
                SetSerialStatus("未发现可用串口，请检查 USB 线与 CH340 驱动", true);
                return;
            }

            int baud;
            if (!int.TryParse(Convert.ToString(BaudCombo.SelectedItem, Invariant), out baud) || baud <= 0)
            {
                baud = 115200;
            }
            OpenSerial(port.Name, baud);
        }

        private void OpenSerial(string portName, int baud)
        {
            if (_replayMode)
            {
                return;
            }
            if (_serial != null && _serial.IsOpen)
            {
                return;
            }

            try
            {
                SerialLink link = new SerialLink();
                link.LineReceived += OnSerialLine;
                link.Faulted += OnSerialFault;
                link.Open(portName, baud);

                _serial = link;
                _frameCount = 0;
                _invalidCount = 0;
                _lineCount = 0;
                _lastFrameAt = DateTime.MinValue;
                _parser = new SonarLineParser();

                ConnectButton.Content = "断开连接";
                ConnectButton.Background = DangerBrush;
                PortCombo.IsEnabled = false;
                BaudCombo.IsEnabled = false;
                RefreshButton.IsEnabled = false;
                ForwardInput.IsReadOnly = true;
                ReverseInput.IsReadOnly = true;

                SetBadge(BadgeLiveBg, BadgeLiveFg, "●  " + portName + " · 实时数据");
                SideModeLabel.Text = "●  串口在线";
                StatusDot.Fill = DotOk;
                SetSerialStatus(string.Format(Invariant, "已连接 {0} · {1} bps，等待数据…", portName, baud), false);
                FrameStats.Text = "已解析 0 帧 · 无效 0";
                AppendRawLine("—— 打开 " + portName + " @ " + baud + " ——");

                _uiTimer.Start();
            }
            catch (Exception ex)
            {
                SetSerialStatus("打开失败：" + DescribeSerialError(ex), true);
            }
        }

        private void DisconnectSerial(string message)
        {
            if (_serial != null)
            {
                SerialLink link = _serial;
                _serial = null;
                try
                {
                    link.LineReceived -= OnSerialLine;
                    link.Faulted -= OnSerialFault;
                    link.Close();
                }
                catch (Exception)
                {
                }
            }

            ConnectButton.Content = "连接串口";
            ConnectButton.Background = AccentBrush;
            PortCombo.IsEnabled = true;
            BaudCombo.IsEnabled = true;
            RefreshButton.IsEnabled = true;

            if (_replayMode)
            {
                return;
            }

            ForwardInput.IsReadOnly = false;
            ReverseInput.IsReadOnly = false;
            SetBadge(BadgeDemoBg, BadgeDemoFg, "●  演示数据 / 未连接串口");
            SideModeLabel.Text = "●  演示模式";
            StatusDot.Fill = DotIdle;
            SetSerialStatus(message == null ? "未连接串口" : message, message != null);
            FrameStats.Text = "—";
        }

        private void OnSerialLine(string line)
        {
            try
            {
                Dispatcher.BeginInvoke(DispatcherPriority.Background, new Action<string>(FeedLine), line);
            }
            catch (Exception)
            {
            }
        }

        private void OnSerialFault(string message)
        {
            try
            {
                Dispatcher.BeginInvoke(DispatcherPriority.Background, new Action<string>(HandleSerialFault), message);
            }
            catch (Exception)
            {
            }
        }

        private void HandleSerialFault(string message)
        {
            if (_serial == null)
            {
                return;
            }
            DisconnectSerial(null);
            SetBadge(BadgeDemoBg, BadgeDemoFg, "●  串口已断开");
            SetSerialStatus("串口断开：" + message, true);
        }

        private static string DescribeSerialError(Exception ex)
        {
            string message = ex.Message;
            if (ex is UnauthorizedAccessException)
            {
                return "串口被占用（SSCOM 等软件是否还开着？）";
            }
            if (ex is ArgumentException)
            {
                return "串口名称无效：" + message;
            }
            return message;
        }

        // ------------------------------------------------------------------
        // 数据接收
        // ------------------------------------------------------------------
        private void FeedLine(string raw)
        {
            if (raw == null)
            {
                return;
            }
            string line = raw.Trim();
            if (line.Length == 0)
            {
                return;
            }

            _lineCount++;
            AppendRawLine(line);

            TemperatureReading temperature;
            if (_tempParser.TryParse(line, out temperature))
            {
                HandleTemperature(temperature);
            }

            SonarFrame frame;
            SonarParseResult result = _parser.TryParse(line, out frame);
            if (result == SonarParseResult.Frame)
            {
                _frameCount++;
                _lastFrameAt = DateTime.Now;
                lock (_frameLock)
                {
                    _pendingFrame = frame;
                }
            }
            else if (result == SonarParseResult.Invalid)
            {
                _invalidCount++;
            }
        }

        private void UiTimer_Tick(object sender, EventArgs e)
        {
            ApplyPendingFrame();
            UpdateConnectionStatus();
            UpdateTemperatureStatus();
            UpdateRecordStatus();
        }

        private void ApplyPendingFrame()
        {
            SonarFrame frame = null;
            lock (_frameLock)
            {
                if (_pendingFrame != null)
                {
                    frame = _pendingFrame;
                    _pendingFrame = null;
                }
            }
            if (frame == null)
            {
                return;
            }

            ForwardInput.Text = (frame.ForwardPs / 1000000.0).ToString("0.000000", Invariant);
            ReverseInput.Text = (frame.BackwardPs / 1000000.0).ToString("0.000000", Invariant);
            Calculate();
            AddRecord(frame);
        }

        private void UpdateConnectionStatus()
        {
            if (_replayMode)
            {
                FrameStats.Text = string.Format(Invariant, "回放 {0} 行 · 解析 {1} 帧 · 无效 {2}",
                    _lineCount, _frameCount, _invalidCount);
                return;
            }
            if (_serial == null || !_serial.IsOpen)
            {
                return;
            }

            string text = string.Format(Invariant, "已连接 {0} · {1} bps", _serial.PortName, _serial.BaudRate);
            bool warn = false;
            if (_lastFrameAt == DateTime.MinValue)
            {
                text += " · 等待数据…";
            }
            else
            {
                double idle = (DateTime.Now - _lastFrameAt).TotalSeconds;
                if (idle > 3.0)
                {
                    text += " · 超过 3 秒无数据";
                    warn = true;
                }
                else
                {
                    text += string.Format(Invariant, " · 最近 {0:0.0} s", idle);
                }
            }

            SetSerialStatus(text, warn);
            FrameStats.Text = string.Format(Invariant, "已解析 {0} 帧 · 无效 {1}", _frameCount, _invalidCount);
        }

        // ------------------------------------------------------------------
        // 流速计算
        // ------------------------------------------------------------------
        private void Calculate()
        {
            try
            {
                double forward = Read(ForwardInput.Text);
                double reverse = Read(ReverseInput.Text);
                double length = Read(LengthInput.Text);
                double angle = Read(AngleInput.Text);

                string zeroText = ZeroInput.Text == null ? "" : ZeroInput.Text.Trim();
                double zero = zeroText.Length == 0 ? 0.0 : Read(zeroText);

                string delayText = DelayInput.Text == null ? "" : DelayInput.Text.Trim();
                double delay = delayText.Length == 0 ? 0.0 : Read(delayText);

                double speed = FlowMath.VelocityMs(forward, reverse, length, angle, zero, delay);

                ForwardValue.Text = forward.ToString("0.000", Invariant);
                ReverseValue.Text = reverse.ToString("0.000", Invariant);
                VelocityValue.Text = speed.ToString("0.000", Invariant);
                DeltaValue.Text = ((reverse - forward) * 1000.0).ToString("0.00", Invariant);
                DirectionLabel.Text = speed > 1e-9 ? "→  正向流动" : speed < -1e-9 ? "←  反向流动" : "—  零流速";
                GeometryLabel.Text = string.Format(Invariant, "声程 {0:g} mm   /   夹角 {1:g}°", length, angle);
                UpdateFlow(speed);
                UpdateTemperaturePanel(forward, reverse, length, angle, zero, delay);

                StatusLabel.Foreground = TextMuted;
                if (_replayMode)
                {
                    StatusLabel.Text = "回放数据已计算 · 时间单位 μs，流速单位 m/s";
                }
                else if (_serial != null && _serial.IsOpen)
                {
                    StatusLabel.Text = "串口数据已自动计算 · 时间单位 μs，流速单位 m/s";
                }
                else
                {
                    StatusLabel.Text = "演示值已计算 · 时间单位 μs，流速单位 m/s";
                }
            }
            catch (FormatException)
            {
                ShowInputError("请输入有效数字，小数点使用“.”。");
            }
            catch (ArgumentException ex)
            {
                ShowInputError(ex.Message);
            }
            catch (OverflowException)
            {
                ShowInputError("输入数值超出可计算范围。");
            }
        }

        private void UpdateFlow(double velocity)
        {
            double pipeId;
            if (!TryParseDouble(PipeIdInput.Text, out pipeId) || pipeId <= 0)
            {
                FlowValue.Text = "—";
                FlowIdLabel.Text = "（D 无效）";
                return;
            }

            double flow = FlowMath.FlowM3h(velocity, pipeId);
            FlowValue.Text = FlowMath.IsFinite(flow)
                ? flow.ToString(Math.Abs(flow) >= 1.0 ? "0.00" : "0.000", Invariant)
                : "—";
            FlowIdLabel.Text = string.Format(Invariant, "（D={0:g} mm）", pipeId);
        }

        private void ShowInputError(string message)
        {
            ForwardValue.Text = "—";
            ReverseValue.Text = "—";
            VelocityValue.Text = "—";
            DeltaValue.Text = "—";
            DirectionLabel.Text = "请检查输入";
            GeometryLabel.Text = "等待有效参数";
            FlowValue.Text = "—";
            FlowIdLabel.Text = "（D 无效）";
            StatusLabel.Foreground = TextBad;
            StatusLabel.Text = message;
            UpdateTemperaturePanel(double.NaN, double.NaN, double.NaN, double.NaN, 0.0, double.NaN);
        }

        // ------------------------------------------------------------------
        // 温度换算
        // ------------------------------------------------------------------
        private void LoadTemperatureCalibration()
        {
            string[] values = TemperatureCalibrationStore.Load();
            bool loaded = false;

            _updatingTempFields = true;
            if (values != null)
            {
                double raw0, t0, raw1, t1;
                if (TryParseDouble(values[0], out raw0) && TryParseDouble(values[1], out t0)
                    && TryParseDouble(values[2], out raw1) && TryParseDouble(values[3], out t1)
                    && TemperatureCalibration.IsUsable(raw0, raw1) && Math.Abs(t1 - t0) > 0.01)
                {
                    Raw0Input.Text = values[0];
                    Temp0Input.Text = values[1];
                    Raw1Input.Text = values[2];
                    Temp1Input.Text = values[3];
                    loaded = true;
                }
            }
            if (!loaded)
            {
                Raw0Input.Text = TemperatureDefaults.Raw0.ToString("0", Invariant);
                Temp0Input.Text = TemperatureDefaults.Temp0C.ToString("0.0", Invariant);
                Raw1Input.Text = TemperatureDefaults.Raw1.ToString("0", Invariant);
                Temp1Input.Text = TemperatureDefaults.Temp1C.ToString("0.0", Invariant);
            }
            _updatingTempFields = false;

            if (loaded)
            {
                CalibrationNote.Text = "已从 temp_calib.ini 读取上次标定值。";
            }
            else if (values != null)
            {
                CalibrationNote.Text = "temp_calib.ini 里的两点标定无效，已改用固件默认值。";
            }
            else
            {
                CalibrationNote.Text = "已按固件默认值预填（0°C = 65097260，100°C = 90163285）。";
            }
        }

        private void SaveTemperatureCalibration()
        {
            if (Raw0Input.Text.Trim().Length == 0 && Raw1Input.Text.Trim().Length == 0)
            {
                return;
            }
            TemperatureCalibrationStore.Save(Raw0Input.Text, Temp0Input.Text, Raw1Input.Text, Temp1Input.Text);
        }

        private void CalibrationDefaults_Click(object sender, RoutedEventArgs e)
        {
            _updatingTempFields = true;
            Raw0Input.Text = TemperatureDefaults.Raw0.ToString("0", Invariant);
            Temp0Input.Text = TemperatureDefaults.Temp0C.ToString("0.0", Invariant);
            Raw1Input.Text = TemperatureDefaults.Raw1.ToString("0", Invariant);
            Temp1Input.Text = TemperatureDefaults.Temp1C.ToString("0.0", Invariant);
            _updatingTempFields = false;
            CalibrationNote.Text = "已恢复固件默认值。";
            RefreshTemperature();
            UpdateTemperaturePanelFromInputs();
        }

        private void Calibration_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!_initialized || _updatingTempFields)
            {
                return;
            }
            RefreshTemperature();
            UpdateTemperaturePanelFromInputs();
        }

        private void RawInput_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!_initialized || _updatingTempFields)
            {
                return;
            }
            _rawFromSerial = false;
            _firmwareTempC = double.NaN;
            RefreshTemperature();
            UpdateTemperaturePanelFromInputs();
        }

        private void CaptureRaw0_Click(object sender, RoutedEventArgs e)
        {
            CaptureRawValue(Raw0Input);
        }

        private void CaptureRaw1_Click(object sender, RoutedEventArgs e)
        {
            CaptureRawValue(Raw1Input);
        }

        private void CaptureRawValue(TextBox target)
        {
            if (!_lastRawTemp.HasValue)
            {
                TempStatus.Text = "还没有原始值，先让设备上报 TEMPC 行";
                return;
            }
            _updatingTempFields = true;
            target.Text = _lastRawTemp.Value.ToString(Invariant);
            _updatingTempFields = false;
            RefreshTemperature();
            UpdateTemperaturePanelFromInputs();
        }

        private void AdoptPath_Click(object sender, RoutedEventArgs e)
        {
            double path;
            if (!TryParseDouble(PathFromTempValue.Text, out path) || !FlowMath.IsFinite(path) || path <= 0)
            {
                TempStatus.Text = "还没有可用的反推声程（需要水温与 Tf/Tr）";
                return;
            }
            LengthInput.Text = path.ToString("0.0", Invariant);
            Calculate();
            StatusLabel.Foreground = TextMuted;
            StatusLabel.Text = string.Format(Invariant, "已按水温反推声程：L = {0:0.0} mm", path);
        }

        private void HandleTemperature(TemperatureReading reading)
        {
            if (reading == null)
            {
                return;
            }

            if (reading.Marker)
            {
                _tempModuleSeen = true;
                UpdateTemperatureStatus();
                return;
            }
            if (reading.Timeout)
            {
                _tempTimeoutAt = DateTime.Now;
                UpdateTemperatureStatus();
                return;
            }

            if (reading.HasRaw)
            {
                _tempTimeoutAt = DateTime.MinValue;
                _lastRawTemp = reading.Raw;
                _lastRawTempAt = DateTime.Now;
                _rawFromSerial = true;
                _firmwareTempC = reading.HasValue ? reading.ValueC : double.NaN;

                _updatingTempFields = true;
                RawInput.Text = reading.Raw.ToString(Invariant);
                _updatingTempFields = false;

                RefreshTemperature();
                UpdateTemperaturePanelFromInputs();
                return;
            }

            if (reading.HasValue)
            {
                _tempTimeoutAt = DateTime.MinValue;
                _firmwareTempC = reading.ValueC;
                _rawFromSerial = true;
                RefreshTemperature();
                UpdateTemperaturePanelFromInputs();
            }
        }

        private void RefreshTemperature()
        {
            long raw;
            bool hasRaw = TryParseRaw(RawInput.Text, out raw);
            _lastRawTemp = hasRaw ? (long?)raw : null;

            double temp = double.NaN;
            double raw0, t0, raw1, t1;
            if (hasRaw && TryReadCalibration(out raw0, out t0, out raw1, out t1))
            {
                temp = TemperatureCalibration.RawToCelsius(raw, raw0, t0, raw1, t1);
            }
            if (!FlowMath.IsFinite(temp) && FlowMath.IsFinite(_firmwareTempC))
            {
                temp = _firmwareTempC;
            }
            _currentTempC = temp;

            UpdateTemperatureStatus();
            UpdateCalibrationSummary();
        }

        private void UpdateTemperatureStatus()
        {
            double raw0, t0, raw1, t1;
            bool calibrated = TryReadCalibration(out raw0, out t0, out raw1, out t1);
            bool hasRaw = _lastRawTemp.HasValue;

            if ((DateTime.Now - _tempTimeoutAt).TotalSeconds < 3.0)
            {
                SetTempBadge(BadgeReplayBg, BadgeReplayFg, "超时");
                TempStatus.Text = "温度测量超时（TEMP,timeout）· 检查 PT1000 接线";
                RawSourceLabel.Text = "未收到数据";
                return;
            }
            if (hasRaw && !calibrated)
            {
                SetTempBadge(BadgeReplayBg, BadgeReplayFg, "未标定");
                TempStatus.Text = string.Format(Invariant, "原始值 {0:N0} · 两点标定无效", _lastRawTemp.Value);
                RawSourceLabel.Text = _rawFromSerial ? "串口自动写入" : "手动输入";
                return;
            }
            if (hasRaw)
            {
                if (_rawFromSerial)
                {
                    SetTempBadge(BadgeLiveBg, BadgeLiveFg, "串口");
                    double age = (DateTime.Now - _lastRawTempAt).TotalSeconds;
                    TempStatus.Text = age < 120.0
                        ? string.Format(Invariant, "原始值 {0:N0} · 最近 {1:0.0} s 前", _lastRawTemp.Value, age)
                        : "温度已超过 2 分钟未更新";
                    RawSourceLabel.Text = "串口自动写入";
                }
                else
                {
                    SetTempBadge(BadgeNeutralBg, BadgeNeutralFg, "本地");
                    TempStatus.Text = string.Format(Invariant, "手动原始值 {0:N0}", _lastRawTemp.Value);
                    RawSourceLabel.Text = "手动输入";
                }
                return;
            }

            SetTempBadge(BadgeNeutralBg, BadgeNeutralFg, "等待");
            TempStatus.Text = _tempModuleSeen ? "温度模块已启用 · 等待 TEMPC 行" : "未收到温度数据";
            RawSourceLabel.Text = "串口或手动输入";
        }

        private void SetTempBadge(Brush background, Brush foreground, string text)
        {
            TempSourceBadge.Background = background;
            TempSourceText.Foreground = foreground;
            TempSourceText.Text = text;
        }

        private void UpdateCalibrationSummary()
        {
            double raw0, t0, raw1, t1;
            if (!TryReadCalibration(out raw0, out t0, out raw1, out t1))
            {
                CalibrationSummary.Text = "两点标定无效：两个原始值不能相同，温度也不能相同。";
                return;
            }
            double slope = Math.Abs((raw1 - raw0) / (t1 - t0));
            CalibrationSummary.Text = string.Format(Invariant,
                "{0:0.0}°C ↔ {1:N0}   ·   {2:0.0}°C ↔ {3:N0}   ·   {4:N0} 计数/°C",
                t0, raw0, t1, raw1, slope);
        }

        private bool TryReadCalibration(out double raw0, out double t0, out double raw1, out double t1)
        {
            raw0 = double.NaN;
            t0 = double.NaN;
            raw1 = double.NaN;
            t1 = double.NaN;
            if (!TryParseDouble(Raw0Input.Text, out raw0)) return false;
            if (!TryParseDouble(Temp0Input.Text, out t0)) return false;
            if (!TryParseDouble(Raw1Input.Text, out raw1)) return false;
            if (!TryParseDouble(Temp1Input.Text, out t1)) return false;
            return TemperatureCalibration.IsUsable(raw0, raw1) && Math.Abs(t1 - t0) > 0.01;
        }

        private void UpdateTemperaturePanelFromInputs()
        {
            double forward, reverse, length, angle, zero, delay;
            TryReadFlowInputs(out forward, out reverse, out length, out angle, out zero, out delay);
            UpdateTemperaturePanel(forward, reverse, length, angle, zero, delay);
        }

        private void TryReadFlowInputs(out double forward, out double reverse, out double length, out double angle, out double zero, out double delay)
        {
            forward = double.NaN;
            reverse = double.NaN;
            length = double.NaN;
            angle = double.NaN;
            zero = 0.0;
            delay = 0.0;
            TryParseDouble(ForwardInput.Text, out forward);
            TryParseDouble(ReverseInput.Text, out reverse);
            TryParseDouble(LengthInput.Text, out length);
            TryParseDouble(AngleInput.Text, out angle);
            double parsedZero;
            if (TryParseDouble(ZeroInput.Text, out parsedZero))
            {
                zero = parsedZero;
            }
            double parsedDelay;
            if (TryParseDouble(DelayInput.Text, out parsedDelay))
            {
                delay = parsedDelay;
            }
        }

        private void UpdateTemperaturePanel(double forwardUs, double reverseUs, double lengthMm, double angleDeg, double zeroNs, double delayUs)
        {
            double tempC = _currentTempC;
            double soundSpeed = FlowMath.IsFinite(tempC) ? WaterTemperature.SoundSpeedMs(tempC) : double.NaN;

            bool hasTemp = FlowMath.IsFinite(soundSpeed) && soundSpeed > 0;
            bool hasTimes = FlowMath.IsFinite(forwardUs) && FlowMath.IsFinite(reverseUs) && forwardUs > 0 && reverseUs > 0;
            bool hasAngle = FlowMath.IsFinite(angleDeg) && angleDeg >= 0 && angleDeg < 90;
            bool hasDelay = FlowMath.IsFinite(delayUs) && delayUs >= 0;

            if (hasTimes && hasDelay && forwardUs > delayUs && reverseUs > delayUs)
            {
                DelayLabel.Text = string.Format(Invariant,
                    "固定延时 d = {0:0.0} μs → Tf′ {1:0.00} μs / Tr′ {2:0.00} μs",
                    delayUs, forwardUs - delayUs, reverseUs - delayUs);
                DelayLabel.Foreground = TextMuted;
            }
            else
            {
                DelayLabel.Text = hasDelay ? "固定延时 d 无效：必须小于 Tf / Tr" : "固定延时 d —";
                DelayLabel.Foreground = TextAmber;
            }

            bool hasValue = FlowMath.IsFinite(tempC);
            string tempText = hasValue ? tempC.ToString("0.0", Invariant) : "—";
            TempValue.Text = tempText;
            ConvertedTempValue.Text = tempText;
            ConvertedSoundSpeed.Text = hasTemp
                ? string.Format(Invariant, "该温度声速 {0:0.0} m/s", soundSpeed)
                : "该温度声速 —";

            if (_lastRawTemp.HasValue)
            {
                TempSubLabel.Text = (_rawFromSerial ? "raw " : "本地 raw ") + _lastRawTemp.Value.ToString(Invariant);
            }
            else
            {
                TempSubLabel.Text = hasValue ? "串口温度" : "等待温度数据";
            }

            if (hasTemp)
            {
                SoundSpeedTValue.Text = soundSpeed.ToString("0.0", Invariant);
                if (hasTimes)
                {
                    PathFromTempValue.Text = WaterTemperature.PathMm(soundSpeed, forwardUs, reverseUs, hasDelay ? delayUs : 0.0).ToString("0.0", Invariant);
                    if (hasAngle)
                    {
                        double velocity = WaterTemperature.VelocityMs(soundSpeed, forwardUs, reverseUs, angleDeg,
                            FlowMath.IsFinite(zeroNs) ? zeroNs : 0.0, hasDelay ? delayUs : 0.0);
                        TempVelocityValue.Text = FlowMath.IsFinite(velocity) ? velocity.ToString("0.000", Invariant) : "—";
                    }
                    else
                    {
                        TempVelocityValue.Text = "—";
                    }
                }
                else
                {
                    PathFromTempValue.Text = "—";
                    TempVelocityValue.Text = "—";
                }
            }
            else
            {
                SoundSpeedTValue.Text = "—";
                PathFromTempValue.Text = "—";
                TempVelocityValue.Text = "—";
            }

            UpdateMeasuredSoundSpeed(forwardUs, reverseUs, lengthMm,
                hasTemp ? soundSpeed : double.NaN,
                hasTemp ? tempC : double.NaN,
                hasDelay ? delayUs : 0.0);

            double raw0, t0, raw1, t1;
            bool calibrated = TryReadCalibration(out raw0, out t0, out raw1, out t1) && Math.Abs(t1 - t0) > 0.01;
            if (calibrated && _lastRawTemp.HasValue)
            {
                DetailRaw.Text = _lastRawTemp.Value.ToString("N0", Invariant);
                DetailDelta.Text = (_lastRawTemp.Value - raw0).ToString("N0", Invariant);
                DetailSlope.Text = Math.Abs((raw1 - raw0) / (t1 - t0)).ToString("N0", Invariant);
                DetailTemp.Text = hasValue ? tempText + " °C" : "—";
            }
            else
            {
                DetailRaw.Text = _lastRawTemp.HasValue ? _lastRawTemp.Value.ToString("N0", Invariant) : "—";
                DetailDelta.Text = "—";
                DetailSlope.Text = "—";
                DetailTemp.Text = "—";
            }
        }

        private void UpdateMeasuredSoundSpeed(double forwardUs, double reverseUs, double lengthMm, double soundSpeedAtTemp, double tempC, double delayUs)
        {
            double measured = FlowMath.SoundSpeedMs(lengthMm, forwardUs, reverseUs, delayUs);
            if (!FlowMath.IsFinite(measured) || measured <= 0)
            {
                SoundSpeedLabel.Text = "实测等效声速 —";
                SoundSpeedLabel.Foreground = TextMuted;
                return;
            }

            if (FlowMath.IsFinite(soundSpeedAtTemp) && soundSpeedAtTemp > 0)
            {
                double path = WaterTemperature.PathMm(soundSpeedAtTemp, forwardUs, reverseUs, delayUs);
                bool consistent = Math.Abs(measured - soundSpeedAtTemp) / soundSpeedAtTemp <= 0.05;
                SoundSpeedLabel.Text = string.Format(Invariant,
                    "实测 c {0:0} m/s · 理论 {1:0} m/s · L≈{2:0.0} mm",
                    measured, soundSpeedAtTemp, path);
                SoundSpeedLabel.Foreground = consistent ? TextGreen : TextAmber;
            }
            else
            {
                bool plausible = measured >= 1300.0 && measured <= 1700.0;
                SoundSpeedLabel.Text = string.Format(Invariant, "实测 c = 2L/(Tf+Tr) = {0:0.0} m/s", measured);
                SoundSpeedLabel.Foreground = plausible ? TextGreen : TextAmber;
            }
        }

        private static bool TryParseRaw(string text, out long raw)
        {
            raw = 0;
            if (text == null)
            {
                return false;
            }
            string trimmed = text.Trim();
            if (trimmed.Length == 0)
            {
                return false;
            }
            double value;
            if (!double.TryParse(trimmed, NumberStyles.Float, Invariant, out value))
            {
                return false;
            }
            if (value < 1.0 || value > 5e9)
            {
                return false;
            }
            raw = (long)Math.Round(value);
            return true;
        }

        private static bool TryParseDouble(string text, out double value)
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
            return double.TryParse(trimmed, NumberStyles.Float, Invariant, out value);
        }

        private static double Read(string value)
        {
            return double.Parse(value, NumberStyles.Float, Invariant);
        }

        private void Calculate_Click(object sender, RoutedEventArgs e)
        {
            Calculate();
        }

        private void Reset_Click(object sender, RoutedEventArgs e)
        {
            ForwardInput.Text = "68.601";
            ReverseInput.Text = "68.728";
            LengthInput.Text = "62";
            AngleInput.Text = "45";
            DelayInput.Text = "27.5";
            PipeIdInput.Text = "15";
            ZeroInput.Text = "0";
            Calculate();
        }

        private void ZeroNow_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double forward = Read(ForwardInput.Text);
                double reverse = Read(ReverseInput.Text);
                if (forward <= 0 || reverse <= 0)
                {
                    throw new ArgumentException("时间必须大于 0。");
                }
                double zero = (reverse - forward) * 1000.0;
                ZeroInput.Text = zero.ToString("0.00", Invariant);
                Calculate();
                StatusLabel.Text = string.Format(Invariant,
                    "已把当前 ΔT = {0:0.00} ns 设为零点，静止水时应显示 0 流速", zero);
            }
            catch (FormatException)
            {
                ShowInputError("请输入有效数字，小数点使用“.”。");
            }
            catch (ArgumentException ex)
            {
                ShowInputError(ex.Message);
            }
        }

        // ------------------------------------------------------------------
        // 界面状态
        // ------------------------------------------------------------------
        private void SetBadge(Brush background, Brush foreground, string text)
        {
            SourceBadge.Background = background;
            SourceBadgeText.Foreground = foreground;
            SourceBadgeText.Text = text;
        }

        private void SetSerialStatus(string text, bool warn)
        {
            SerialStatus.Text = text;
            SerialStatus.Foreground = warn ? TextBad : TextMuted;
            _statusLocked = warn;
            if (warn)
            {
                StatusDot.Fill = DotBad;
            }
            else if (_serial != null && _serial.IsOpen)
            {
                StatusDot.Fill = DotOk;
            }
            else
            {
                StatusDot.Fill = DotIdle;
            }
        }

        private void AppendRawLine(string line)
        {
            _rawLines.Add(line);
            while (_rawLines.Count > RawLineLimit)
            {
                _rawLines.RemoveAt(0);
            }
            RawLog.Text = string.Join(Environment.NewLine, _rawLines.ToArray());
            RawLog.ScrollToEnd();
            RawHint.Text = string.Format(Invariant, "显示最近 {0} 行 · 共收到 {1} 行", _rawLines.Count, _lineCount);
        }

        private void ClearRaw_Click(object sender, RoutedEventArgs e)
        {
            _rawLines.Clear();
            RawLog.Text = "";
            RawHint.Text = "未接收数据";
        }

        // ------------------------------------------------------------------
        // 记录 / CSV 导出
        // ------------------------------------------------------------------
        private string GetRecordFolder()
        {
            return Path.Combine(AppContext.BaseDirectory, "记录");
        }

        private void Record_Click(object sender, RoutedEventArgs e)
        {
            if (_recordWriter != null)
            {
                StopRecording("已停止记录");
            }
            else
            {
                StartRecording();
            }
        }

        private void StartRecording()
        {
            try
            {
                string folder = GetRecordFolder();
                Directory.CreateDirectory(folder);
                string path = Path.Combine(folder, "LS1D_" + DateTime.Now.ToString("yyyyMMdd_HHmmss", Invariant) + ".csv");

                StreamWriter writer = new StreamWriter(path, false, new UTF8Encoding(true));
                writer.AutoFlush = true;
                writer.WriteLine(RecordHeader);

                _recordWriter = writer;
                _recordPath = path;
                _recordStartedAt = DateTime.Now;

                RecordButton.Content = "停止记录";
                RecordBadge.Background = BadgeLiveBg;
                RecordBadgeText.Foreground = BadgeLiveFg;
                RecordBadgeText.Text = "记录中";
                RecordPathLabel.Text = "记录文件：" + Path.Combine("记录", Path.GetFileName(path));
                RecordStatus.Text = "记录中 · 已记录 0 条";
                UpdateRecordPreview();
            }
            catch (Exception ex)
            {
                RecordStatus.Text = "无法创建记录文件：" + ex.Message;
            }
        }

        private void StopRecording(string message)
        {
            CloseRecordWriter();
            RecordButton.Content = "开始记录";
            RecordBadge.Background = BadgeNeutralBg;
            RecordBadgeText.Foreground = BadgeNeutralFg;
            RecordBadgeText.Text = "未记录";
            if (!string.IsNullOrEmpty(_recordPath))
            {
                RecordPathLabel.Text = "记录文件：" + Path.Combine("记录", Path.GetFileName(_recordPath));
            }
            RecordStatus.Text = (message == null ? "已停止记录" : message)
                + string.Format(Invariant, " · 共 {0} 条", _records.Count);
        }

        private void CloseRecordWriter()
        {
            if (_recordWriter == null)
            {
                return;
            }
            try
            {
                _recordWriter.Flush();
                _recordWriter.Dispose();
            }
            catch (Exception)
            {
            }
            _recordWriter = null;
        }

        private void UpdateRecordStatus()
        {
            if (_recordWriter != null)
            {
                RecordStatus.Text = string.Format(Invariant, "记录中 · 已记录 {0} 条", _records.Count);
            }
        }

        private void AddRecord(SonarFrame frame)
        {
            if (frame == null || frame.ForwardPs <= 0 || frame.BackwardPs <= 0)
            {
                return;
            }

            double tf = frame.ForwardPs / 1000000.0;
            double tr = frame.BackwardPs / 1000000.0;

            double ignoredForward, ignoredReverse, length, angle, zero, delay;
            TryReadFlowInputs(out ignoredForward, out ignoredReverse, out length, out angle, out zero, out delay);

            double velocity = double.NaN;
            try
            {
                velocity = FlowMath.VelocityMs(tf, tr, length, angle, zero, delay);
            }
            catch (ArgumentException)
            {
            }

            double soundSpeed = FlowMath.SoundSpeedMs(length, tf, tr, delay);
            double theorySpeed = WaterTemperature.SoundSpeedMs(_currentTempC);
            double deltaNs = (tr - tf) * 1000.0 - zero;

            double pipeId;
            if (!TryParseDouble(PipeIdInput.Text, out pipeId))
            {
                pipeId = double.NaN;
            }
            double flow = FlowMath.FlowM3h(velocity, pipeId);

            DateTime now = DateTime.Now;
            string line = string.Join(",", new string[]
            {
                now.ToString("yyyy-MM-dd HH:mm:ss.fff", Invariant),
                frame.HasSequence ? frame.Sequence.ToString(Invariant) : "",
                tf.ToString("0.000000", Invariant),
                tr.ToString("0.000000", Invariant),
                FormatRecordValue(deltaNs, "0.00"),
                FormatRecordValue(velocity, "0.000"),
                FormatRecordValue(_currentTempC, "0.0"),
                _lastRawTemp.HasValue ? _lastRawTemp.Value.ToString(Invariant) : "",
                frame.HasDtof ? frame.Dtof.ToString(Invariant) : "",
                FormatRecordValue(soundSpeed, "0.0"),
                FormatRecordValue(theorySpeed, "0.0"),
                FormatRecordValue(length, "0.0"),
                FormatRecordValue(angle, "0.0"),
                FormatRecordValue(delay, "0.0"),
                FormatRecordValue(pipeId, "0.0"),
                FormatRecordValue(flow, "0.000"),
                FormatRecordValue(zero, "0.0")
            });

            _records.Add(new MeasureRecord { Time = now, Line = line });
            if (_records.Count > 50000)
            {
                _records.RemoveRange(0, 10000);
            }

            if (_recordWriter != null)
            {
                try
                {
                    _recordWriter.WriteLine(line);
                }
                catch (Exception ex)
                {
                    CloseRecordWriter();
                    RecordStatus.Text = "写入失败：" + ex.Message;
                }
            }
            UpdateRecordPreview();
        }

        private static string FormatRecordValue(double value, string format)
        {
            return FlowMath.IsFinite(value) ? value.ToString(format, Invariant) : "";
        }

        private void UpdateRecordPreview()
        {
            StringBuilder builder = new StringBuilder();
            builder.AppendLine(RecordHeader);
            int start = Math.Max(0, _records.Count - 20);
            for (int i = start; i < _records.Count; i++)
            {
                builder.AppendLine(_records[i].Line);
            }
            RecordPreview.Text = builder.ToString().TrimEnd();
            RecordPreview.ScrollToEnd();
        }

        private void Export_Click(object sender, RoutedEventArgs e)
        {
            if (_records.Count == 0)
            {
                RecordStatus.Text = "还没有测量记录，先连接串口或回放数据";
                return;
            }

            SaveFileDialog dialog = new SaveFileDialog();
            dialog.Title = "导出测量数据";
            dialog.Filter = "CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*";
            dialog.FileName = "LS1D记录_" + DateTime.Now.ToString("yyyyMMdd_HHmmss", Invariant) + ".csv";
            try
            {
                Directory.CreateDirectory(GetRecordFolder());
            }
            catch (Exception)
            {
            }
            dialog.InitialDirectory = GetRecordFolder();
            if (dialog.ShowDialog(this) != true)
            {
                return;
            }

            try
            {
                using (StreamWriter writer = new StreamWriter(dialog.FileName, false, new UTF8Encoding(true)))
                {
                    writer.WriteLine(RecordHeader);
                    for (int i = 0; i < _records.Count; i++)
                    {
                        writer.WriteLine(_records[i].Line);
                    }
                }
                RecordStatus.Text = string.Format(Invariant, "已导出 {0} 条到 {1}", _records.Count, dialog.FileName);
            }
            catch (Exception ex)
            {
                RecordStatus.Text = "导出失败：" + ex.Message;
            }
        }

        private void OpenRecordFolder_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string folder = GetRecordFolder();
                Directory.CreateDirectory(folder);
                if (!string.IsNullOrEmpty(_recordPath) && File.Exists(_recordPath))
                {
                    Process.Start("explorer.exe", "/select,\"" + _recordPath + "\"");
                }
                else
                {
                    Process.Start("explorer.exe", folder);
                }
            }
            catch (Exception ex)
            {
                RecordStatus.Text = "无法打开文件夹：" + ex.Message;
            }
        }

        // ------------------------------------------------------------------
        // 日志回放 / 截图
        // ------------------------------------------------------------------
        private void StartReplay(string path)
        {
            string[] lines;
            try
            {
                lines = File.ReadAllLines(path);
            }
            catch (Exception ex)
            {
                SetBadge(BadgeDemoBg, BadgeDemoFg, "●  回放失败");
                SetSerialStatus("无法读取回放文件：" + ex.Message, true);
                return;
            }

            _replayMode = true;
            _parser = new SonarLineParser();
            SetBadge(BadgeReplayBg, BadgeReplayFg, "●  日志回放 · 测试数据");
            SideModeLabel.Text = "●  回放模式";
            StatusDot.Fill = MakeBrush(0xD9, 0x8A, 0x00);
            SerialStatus.Foreground = TextMuted;
            SerialStatus.Text = "日志回放：" + Path.GetFileName(path);
            FrameStats.Text = "等待回放…";
            ForwardInput.IsReadOnly = true;
            ReverseInput.IsReadOnly = true;

            int index = 0;
            _replayTimer = new DispatcherTimer();
            _replayTimer.Interval = TimeSpan.FromMilliseconds(90);
            _replayTimer.Tick += delegate
            {
                while (index < lines.Length && lines[index].Trim().Length == 0)
                {
                    index++;
                }
                if (index >= lines.Length)
                {
                    _replayTimer.Stop();
                    FrameStats.Text = string.Format(Invariant, "回放结束 · {0} 帧", _frameCount);
                    return;
                }
                FeedLine(lines[index]);
                index++;
            };
            _replayTimer.Start();
        }

        private void ScheduleScreenshot(int delayMs)
        {
            DispatcherTimer timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromMilliseconds(delayMs);
            timer.Tick += delegate
            {
                timer.Stop();
                BeginScreenshot();
            };
            timer.Start();
        }

        private void BeginScreenshot()
        {
            Dispatcher.BeginInvoke(DispatcherPriority.ContextIdle, new Action(CaptureAndClose));
        }

        private void CaptureAndClose()
        {
            try
            {
                FrameworkElement surface = AppRoot;
                double width = surface.ActualWidth;
                double height = surface.ActualHeight;
                if (width <= 0 || height <= 0)
                {
                    throw new InvalidOperationException("窗口尚未完成布局。");
                }

                RenderTargetBitmap bitmap = RenderElement(surface, width, height);
                PngBitmapEncoder encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create(bitmap));
                using (FileStream file = File.Create(Path.Combine(AppContext.BaseDirectory, "preview.png")))
                {
                    encoder.Save(file);
                }
            }
            catch (Exception ex)
            {
                try
                {
                    File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "preview_error.txt"), ex.ToString());
                }
                catch (Exception)
                {
                }
            }
            finally
            {
                Close();
            }
        }

        private static RenderTargetBitmap RenderElement(FrameworkElement element, double width, double height)
        {
            RenderTargetBitmap bitmap = new RenderTargetBitmap(
                (int)Math.Ceiling(width), (int)Math.Ceiling(height), 96, 96, PixelFormats.Pbgra32);
            bitmap.Render(element);
            return bitmap;
        }
    }
}
