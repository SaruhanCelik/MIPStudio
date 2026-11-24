param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [string]$EvidenceDirectory
)

# Integration check for the real packaged Windows executable. Only windows owned
# by the process this script launches are observed/captured/closed.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($EvidenceDirectory)) {
    $EvidenceDirectory = Join-Path (Get-Location) 'verification'
}
$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$evidencePath = [IO.Path]::GetFullPath($EvidenceDirectory)
New-Item -ItemType Directory -Force -Path $evidencePath | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;

public sealed class StartupWindow {
    public IntPtr Handle;
    public string Title;
}

public static class StartupProbe {
    private delegate bool WindowCallback(IntPtr handle, IntPtr argument);
    [DllImport("user32.dll")] private static extern bool EnumWindows(WindowCallback callback, IntPtr argument);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr handle, out uint processId);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr handle);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetWindowText(IntPtr handle, StringBuilder text, int count);
    [DllImport("user32.dll")] private static extern bool GetWindowRect(IntPtr handle, out WindowRect rectangle);
    [DllImport("user32.dll")] private static extern bool PrintWindow(IntPtr handle, IntPtr device, uint flags);
    [DllImport("user32.dll")] private static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)] public static extern uint ExtractIconEx(string filename, int index, IntPtr[] large, IntPtr[] small, uint count);
    [StructLayout(LayoutKind.Sequential)] private struct WindowRect { public int Left, Top, Right, Bottom; }

    public static StartupWindow[] Windows(int processId) {
        var windows = new List<StartupWindow>();
        EnumWindows(delegate(IntPtr handle, IntPtr argument) {
            uint owner;
            GetWindowThreadProcessId(handle, out owner);
            if (owner == processId && IsWindowVisible(handle)) {
                var text = new StringBuilder(1024);
                GetWindowText(handle, text, text.Capacity);
                if (text.Length > 0)
                    windows.Add(new StartupWindow { Handle = handle, Title = text.ToString() });
            }
            return true;
        }, IntPtr.Zero);
        return windows.ToArray();
    }

    public static bool Capture(IntPtr handle, string filename) {
        var oldContext = SetThreadDpiAwarenessContext(new IntPtr(-4));
        try {
            WindowRect rectangle;
            if (!GetWindowRect(handle, out rectangle)) return false;
            using (var bitmap = new Bitmap(rectangle.Right - rectangle.Left, rectangle.Bottom - rectangle.Top))
            using (var graphics = Graphics.FromImage(bitmap)) {
                var device = graphics.GetHdc();
                bool captured;
                try { captured = PrintWindow(handle, device, 2); }
                finally { graphics.ReleaseHdc(device); }
                if (captured) bitmap.Save(filename, ImageFormat.Png);
                return captured;
            }
        } finally { SetThreadDpiAwarenessContext(oldContext); }
    }
}
'@

$process = [Diagnostics.Process]::new()
$process.StartInfo.FileName = $executablePath
$process.StartInfo.WorkingDirectory = $evidencePath # Deliberately not the package directory.
$process.StartInfo.UseShellExecute = $false
$process.StartInfo.RedirectStandardOutput = $true
$process.StartInfo.RedirectStandardError = $true
# Prove dependency discovery does not rely on the installed Qt or MinGW PATH.
$process.StartInfo.EnvironmentVariables['PATH'] = "$env:SystemRoot/System32;$env:SystemRoot"
foreach ($variable in @('QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QT_QPA_FONTDIR', 'QT_SCALE_FACTOR', 'QTDIR', 'QML2_IMPORT_PATH')) {
    $process.StartInfo.EnvironmentVariables.Remove($variable)
}
$process.StartInfo.EnvironmentVariables['QT_QPA_PLATFORM'] = 'windows'
$process.StartInfo.EnvironmentVariables['QT_LOGGING_RULES'] = 'mipstudio.startup.info=true'
$process.StartInfo.EnvironmentVariables['QT_FORCE_STDERR_LOGGING'] = '1'

$firstSplashMs = $null
$firstMainMs = $null
$splashCaptured = $false
$mainCaptured = $false
$windowTitles = [Collections.Generic.List[string]]::new()
$clock = [Diagnostics.Stopwatch]::StartNew()
$processStarted = $false
try {
    if (-not $process.Start()) { throw 'Could not launch the packaged executable.' }
    $processStarted = $true
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    while ($clock.ElapsedMilliseconds -lt 20000 -and -not $process.HasExited) {
        $ownWindows = [StartupProbe]::Windows($process.Id)
        foreach ($window in $ownWindows) {
            if (-not $windowTitles.Contains($window.Title)) { $windowTitles.Add($window.Title) }
            if ($window.Title -like 'MIPStudio*Starting*') {
                if ($null -eq $firstSplashMs) { $firstSplashMs = $clock.ElapsedMilliseconds }
                if (-not $splashCaptured -and $clock.ElapsedMilliseconds - $firstSplashMs -ge 400) {
                    $splashCaptured = [StartupProbe]::Capture($window.Handle, (Join-Path $evidencePath 'release-splash.png'))
                }
            } elseif ($window.Title -like '*MIPS Studio*') {
                if ($null -eq $firstSplashMs) { throw 'The main window appeared before any splash was observed.' }
                $firstMainMs = $clock.ElapsedMilliseconds
                [Threading.Thread]::Sleep(200) # Test process only; the application event loop is independent.
                $mainCaptured = [StartupProbe]::Capture($window.Handle, (Join-Path $evidencePath 'release-main-window.png'))
                break
            } elseif ($null -eq $firstMainMs) {
                throw "Unexpected visible startup window: $($window.Title)"
            }
        }
        if ($null -ne $firstMainMs) { break }
        [Threading.Thread]::Sleep(5)
    }
    if ($null -eq $firstMainMs) { throw 'Startup did not reach the main window within 20 seconds.' }
    if (-not $splashCaptured -or -not $mainCaptured) { throw 'Could not capture the splash and main window for visual review.' }
    if (-not $process.CloseMainWindow() -or -not $process.WaitForExit(5000)) {
        throw 'The application did not close cleanly.'
    }
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    $stdout + $stderr | Set-Content -Encoding utf8 (Join-Path $evidencePath 'release-startup.log')
    $visibleMatch = [regex]::Match($stderr, 'event=splash-ready visible_ms=(\d+)')
    if (-not $visibleMatch.Success) { throw 'The executable did not report its measured splash lifetime.' }
    $visibleMs = [long]$visibleMatch.Groups[1].Value
    if ($visibleMs -lt 4500) { throw "Splash lifetime was only $visibleMs ms." }
    if (-not $stderr.Contains('event=main-window-shown')) { throw 'Main-window transition was not reported.' }
    if ($process.ExitCode -ne 0) { throw "Application exited with code $($process.ExitCode)." }
    $iconCount = [StartupProbe]::ExtractIconEx($executablePath, -1, $null, $null, 0)
    if ($iconCount -lt 1) { throw 'The executable has no extractable Windows icon resource.' }
    $result = [ordered]@{
        status = 'passed'
        verifiedUtc = [DateTime]::UtcNow.ToString('o')
        executable = $executablePath
        sha256 = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash
        measuredSplashMs = $visibleMs
        observedSplashToMainMs = $firstMainMs - $firstSplashMs
        iconResourceGroups = $iconCount
        exitCode = $process.ExitCode
        qtAndMinGWRemovedFromPath = $true
        windowTitles = $windowTitles.ToArray()
    }
    $result | ConvertTo-Json -Depth 4 | Tee-Object -FilePath (Join-Path $evidencePath 'release-startup-result.json')
} finally {
    if ($processStarted -and -not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    if ($processStarted) {
        $stdoutTask.GetAwaiter().GetResult() + $stderrTask.GetAwaiter().GetResult() |
            Set-Content -Encoding utf8 (Join-Path $evidencePath 'release-startup.log')
    }
    $process.Dispose()
}
