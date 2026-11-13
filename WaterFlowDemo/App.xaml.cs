using System;
using System.Windows;

namespace WaterFlowDemo
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            // 自检模式：不创建窗口，跑完解析/公式用例后直接退出（退出码 0=全部通过）
            if (Array.IndexOf(e.Args, "--selftest") >= 0)
            {
                Shutdown(SelfTest.Run());
                return;
            }

            MainWindow window = new MainWindow();
            MainWindow = window;
            window.Show();
        }
    }
}
