using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Text;
using System.Threading;

namespace WaterFlowDemo
{
    /// <summary>
    /// 串口读取封装：后台线程阻塞读，按行切分后抛 LineReceived。
    /// 与 UI 完全解耦，断开/拔线时不会抛到界面线程。
    /// </summary>
    public sealed class SerialLink : IDisposable
    {
        private readonly object _sync = new object();
        private readonly StringBuilder _buffer = new StringBuilder();
        private readonly byte[] _readBuffer = new byte[1024];
        private SerialPort _port;
        private Thread _reader;
        private volatile bool _running;
        private DateTime _lastDataAt = DateTime.MinValue;

        public event Action<string> LineReceived;
        public event Action<string> Faulted;

        public bool IsOpen { get; private set; }
        public string PortName { get; private set; }
        public int BaudRate { get; private set; }

        public void Open(string portName, int baudRate)
        {
            if (IsOpen)
            {
                throw new InvalidOperationException("串口已经打开。");
            }
            if (string.IsNullOrEmpty(portName))
            {
                throw new ArgumentException("串口名称为空。");
            }

            SerialPort port = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One);
            port.ReadTimeout = 200;
            port.WriteTimeout = 500;
            port.DtrEnable = false;
            port.RtsEnable = false;
            port.Encoding = Encoding.ASCII;
            port.Open();

            _port = port;
            PortName = portName;
            BaudRate = baudRate;
            IsOpen = true;
            _running = true;
            _lastDataAt = DateTime.Now;

            _reader = new Thread(ReadLoop);
            _reader.IsBackground = true;
            _reader.Name = "SonarSerialReader";
            _reader.Start();
        }

        private void ReadLoop()
        {
            while (_running)
            {
                SerialPort port = _port;
                if (port == null)
                {
                    break;
                }
                try
                {
                    int count = port.Read(_readBuffer, 0, _readBuffer.Length);
                    if (count > 0)
                    {
                        _lastDataAt = DateTime.Now;
                        Append(Encoding.ASCII.GetString(_readBuffer, 0, count));
                    }
                    else
                    {
                        Thread.Sleep(10);
                    }
                }
                catch (TimeoutException)
                {
                    FlushStale();
                }
                catch (Exception ex)
                {
                    if (_running)
                    {
                        RaiseFault(ex.Message);
                    }
                    return;
                }
            }
        }

        private void Append(string text)
        {
            List<string> lines;
            lock (_sync)
            {
                _buffer.Append(text);
                if (_buffer.Length > 16384)
                {
                    _buffer.Length = 0;   // 长时间无换行，防止缓冲无限增长
                }
                lines = ExtractLines();
            }

            for (int i = 0; i < lines.Count; i++)
            {
                RaiseLine(lines[i]);
            }
        }

        private List<string> ExtractLines()
        {
            List<string> lines = new List<string>();
            string text = _buffer.ToString().Replace("\r\n", "\n").Replace('\r', '\n');
            int index;
            while ((index = text.IndexOf('\n')) >= 0)
            {
                string line = text.Substring(0, index).Trim();
                text = text.Substring(index + 1);
                if (line.Length > 0)
                {
                    lines.Add(line);
                }
            }
            _buffer.Length = 0;
            _buffer.Append(text);
            return lines;
        }

        private void FlushStale()
        {
            string line = null;
            lock (_sync)
            {
                if (_buffer.Length > 0 && (DateTime.Now - _lastDataAt).TotalMilliseconds > 400)
                {
                    line = _buffer.ToString().Trim();
                    _buffer.Length = 0;
                }
            }
            if (!string.IsNullOrEmpty(line))
            {
                RaiseLine(line);
            }
        }

        private void RaiseLine(string line)
        {
            Action<string> handler = LineReceived;
            if (handler == null)
            {
                return;
            }
            try
            {
                handler(line);
            }
            catch
            {
                // 界面关闭后 Dispatcher 可能已经不可用，忽略即可
            }
        }

        private void RaiseFault(string message)
        {
            Action<string> handler = Faulted;
            if (handler == null)
            {
                return;
            }
            try
            {
                handler(message);
            }
            catch
            {
            }
        }

        public void Close()
        {
            _running = false;
            IsOpen = false;

            SerialPort port = _port;
            _port = null;
            if (port != null)
            {
                try { port.Close(); }
                catch { }
                try { port.Dispose(); }
                catch { }
            }

            Thread reader = _reader;
            _reader = null;
            if (reader != null && reader != Thread.CurrentThread)
            {
                try { reader.Join(600); }
                catch { }
            }
        }

        public void Dispose()
        {
            Close();
        }
    }
}
