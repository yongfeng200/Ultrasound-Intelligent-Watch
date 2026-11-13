using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Management;

namespace WaterFlowDemo
{
    public sealed class PortInfo
    {
        public string Name { get; set; }
        public string Description { get; set; }

        public override string ToString()
        {
            if (string.IsNullOrEmpty(Name))
            {
                return string.IsNullOrEmpty(Description) ? "（未发现串口）" : Description;
            }
            if (string.IsNullOrEmpty(Description))
            {
                return Name;
            }
            return Name + " · " + Description;
        }
    }

    /// <summary>
    /// 列出本机串口，并尽量用 WMI 取出 USB-SERIAL CH340 之类的友好名称。
    /// WMI 不可用时只返回 COM 名称，不影响使用。
    /// </summary>
    public static class SerialPorts
    {
        public static List<PortInfo> GetPorts()
        {
            List<PortInfo> result = new List<PortInfo>();
            string[] names;
            try
            {
                names = SerialPort.GetPortNames();
            }
            catch
            {
                names = new string[0];
            }
            Array.Sort(names, StringComparer.OrdinalIgnoreCase);

            Dictionary<string, string> descriptions = SafeWmiDescriptions();
            for (int i = 0; i < names.Length; i++)
            {
                PortInfo info = new PortInfo();
                info.Name = names[i];
                string description;
                if (descriptions.TryGetValue(names[i], out description))
                {
                    info.Description = description;
                }
                else
                {
                    info.Description = "";
                }
                result.Add(info);
            }
            return result;
        }

        private static Dictionary<string, string> SafeWmiDescriptions()
        {
            Dictionary<string, string> map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            try
            {
                using (ManagementObjectSearcher searcher = new ManagementObjectSearcher(
                    "SELECT Caption FROM Win32_PnPEntity WHERE Caption LIKE '%(COM%'"))
                using (ManagementObjectCollection found = searcher.Get())
                {
                    foreach (ManagementBaseObject item in found)
                    {
                        object captionValue = item["Caption"];
                        if (captionValue == null)
                        {
                            continue;
                        }
                        string caption = captionValue.ToString();
                        int left = caption.LastIndexOf("(COM", StringComparison.OrdinalIgnoreCase);
                        if (left < 0)
                        {
                            continue;
                        }
                        int right = caption.IndexOf(')', left);
                        if (right < 0)
                        {
                            continue;
                        }
                        string portName = caption.Substring(left + 1, right - left - 1).Trim();
                        string deviceName = caption.Substring(0, left).Trim();
                        if (portName.Length > 0)
                        {
                            map[portName] = deviceName;
                        }
                    }
                }
            }
            catch
            {
                // 没有 WMI 权限或服务时静默降级
            }
            return map;
        }
    }
}
