# HmosUsbDisplay

通过 USB 数据线把鸿蒙平板用作 Windows 的独立扩展屏。Windows 发送端创建 Parsec 虚拟显示器，将其画面编码为 H.264，经华为 HDC 的 USB 转发通道发给平板端 SubScreen。

已在 Windows 11 25H2 和 HarmonyOS 7.0.0.111 SP18 的 LRT-W20 平板上完成 1920×1080 短时真机验证。Intel Quick Sync 编码时发送进程的提交内存持续增长，长时间运行仍待验证。详见[真机验证记录](docs/usb-verification.md)。

## 目录

| 路径 | 内容 |
| --- | --- |
| `pc/` | Windows 发送端、构建脚本和诊断工具 |
| `harmony/` | 原生鸿蒙接收端和公开的构建配置模板 |
| `docs/usb-verification.md` | 短时真机结果和已知限制 |
| `workboard.html` | 项目任务看板，可在本地浏览器打开 |

## 构建 Windows 发送端

需要支持 AVX2 的 Windows x64 电脑、Visual Studio 2022 的 **MSVC x64/x86 生成工具**和 Windows SDK 10.0.26100。扩展模式需要安装 [Parsec Virtual Display Driver](https://support.parsec.app/hc/en-us/articles/32381178803604-VDD-Overview-Prerequisites-and-Installation)。

在 PowerShell 7 中切换到仓库根目录，运行以下命令生成 `pc/subscreen_sender.exe`。

```powershell
& .\pc\build.ps1
```

## 构建鸿蒙接收端

使用 DevEco Studio 26.0.0（内置 HarmonyOS SDK API 26）。在仓库根目录为本机创建构建配置并打开 `harmony/` 工程。

```powershell
if (-not (Test-Path -LiteralPath .\harmony\build-profile.json5)) {
    Copy-Item -LiteralPath .\harmony\build-profile.example.json5 -Destination .\harmony\build-profile.json5
}
```

在 DevEco Studio 同步工程。真机调试时，连接平板并授权 USB 调试，在「文件 → 项目结构 → 项目 → 签名配置」生成自己的调试签名，为 `default` 产品选用 `default` 签名配置。保存后可通过 DevEco Studio 运行 `entry`。本机签名配置和生成的 HAP 均由 Git 忽略。

也可以用以下 PowerShell 命令构建。签名工具使用 DevEco 随附的 Java，构建产物位于 `harmony/entry/build/default/outputs/default/`。

```powershell
$devEco = Read-Host 'DevEco Studio 安装目录'
$env:DEVECO_SDK_HOME = Join-Path $devEco 'sdk'
$env:JAVA_HOME = Join-Path $devEco 'jbr'
$env:Path = "$env:JAVA_HOME\bin;$devEco\tools\node;$devEco\tools\ohpm\bin;$env:Path"

Push-Location .\harmony
try {
    & (Join-Path $devEco 'tools\ohpm\bin\ohpm.bat') install
    if ($LASTEXITCODE -ne 0) { throw '鸿蒙依赖安装失败' }
    & (Join-Path $devEco 'tools\node\node.exe') (Join-Path $devEco 'tools\hvigor\bin\hvigorw.js') assembleHap --mode module -p product=default -p module=entry@default --no-daemon
    if ($LASTEXITCODE -ne 0) { throw '鸿蒙接收端构建失败' }
} finally {
    Pop-Location
}
```

## 验证 USB 扩展屏

在平板打开已安装的 SubScreen。用支持数据传输的 USB 线连接两端，确认 `hdc list targets -v` 显示预期平板为 `USB`、`Connected`。以下命令会运行一次 20 秒的 1920×1080 扩展屏测试。测试期间，把一个 Windows 测试窗口移到新增的显示器，在平板上核对画面。

```powershell
$devEco = Read-Host 'DevEco Studio 安装目录'
$hdc = Join-Path $devEco 'sdk\default\openharmony\toolchains\hdc.exe'
& $hdc list targets -v
if ((& $hdc fport ls) -match 'tcp:53517') { throw '端口 53517 已有转发' }
$forward = & $hdc fport tcp:53517 tcp:53517
if (($forward -join ' ') -notmatch 'result:OK') { throw ($forward -join ' ') }

try {
    $sender = (Resolve-Path -LiteralPath .\pc\subscreen_sender.exe).Path
    $test = Start-Process -FilePath $sender -ArgumentList @('--extend', '1920x1080@60', '--fps', '25', '--duration', '20', '--nohdc') -Wait -PassThru
    if ($test.ExitCode -ne 0) { throw "Windows 发送端返回 $($test.ExitCode)" }
} finally {
    & $hdc fport rm tcp:53517 tcp:53517
}
```

## 来源与许可

源码基于 [InChange-Jiang/HarmonyOS-Subscreen 的 `a070d89` 提交](https://github.com/InChange-Jiang/HarmonyOS-Subscreen/tree/a070d8910e0b6ed79d202cd2adefb8b23ca70a23)。[LICENSE](LICENSE) 保留上游 MIT 许可证及版权。`pc/third_party/parsec-vdd.h` 内保留 Nguyen Duy 的版权与许可声明。DevEco Studio、Windows 工具链和 Parsec 驱动由各自厂商提供。
