# 公开仓库准备独立核验要求

核验对象为 HmosUsbDisplay 的公开仓库准备阶段。上游基线是 SubScreen 的 `a070d8910e0b6ed79d202cd2adefb8b23ca70a23` 提交。请在新对话中独立核对当前工作区，按下面的检查顺序记录证据、发现和复核结果。

## 材料

- [项目说明](../README.md)、[工作看板](../workboard.html)、[真机验证记录](usb-verification.md)和 [MIT 许可证](../LICENSE)。
- Windows 发送端位于 `pc/`。鸿蒙接收端位于 `harmony/`，公开配置模板为 `harmony/build-profile.example.json5`。
- 本机校验材料位于 `.local/`。原始上游 ZIP 位于 `.local/upstream/SubScreen-source.zip`，截图、日志及清理记录位于 `.local/evidence/2026-11-24/`。

## 检查项目与验收条件

1. **提交范围。** 在当前本机工作区通过独立 Git 目录生成候选清单。在已克隆的公开副本里，使用对应仓库的 `git ls-files` 检查同一范围。候选范围应包括 `pc/`、`harmony/` 的源码、构建入口、Hvigor 配置、许可证和项目文档。检查文件内容是否含本机绝对路径、设备唯一标识或签名材料。

   ```powershell
   git --git-dir .\.local\repo-prep-git --work-tree . ls-files --cached --others --exclude-standard
   git --git-dir .\.local\repo-prep-git --work-tree . check-ignore -v -- harmony/build-profile.json5 harmony/entry/build/default/outputs/default/entry-default-signed.hap .local/evidence/2026-11-24/subscreen-usb-proof-154606.jpeg
   ```

   预期本机签名配置、证书、签名安装包、构建缓存、编译产物、安装器和原始验证文件都在忽略范围内。签名检查只记录文件路径和是否通过。

2. **源码与许可。** 使用本机上游 ZIP 或提交链接核对源码与原版权。比较运行源码、MIT 许可证和第三方头文件的原始内容。公开仓库中可复现的构建配置变化应能从上游版本和当前说明辨认。
3. **重新构建。** 按 README 在 PowerShell 7 中运行 Windows 构建。用公开配置模板在干净副本中构建未签名 HAP，并在具备本机签名材料时验证签名构建。确认两类 HAP 出现在各自构建目录。
4. **核对调试流程。** 检查本机 USB 回归材料。可用真机时，确认 HDC 连接、签名包安装、Windows 额外显示器和扩展画面，并核对退出后回收虚拟显示器和临时端口转发。记录设备状态、分辨率与结果。
5. **目录与记录。** 核对 `.local/evidence/2026-11-24/repo-prep-cleanup.json`、原始证据和看板链接。验证清理项确为可重建或有来源的临时文件，核对当前保留的回滚副本和本机签名配置。检查看板的独立核验、用户审阅、可选经验整理与收尾清理顺序。

## 核验回执

逐项记录通过、发现问题或缺少现场条件，并附证据。发现问题时记录相关文件和复现步骤。修正后再记录复核结果，供用户审阅阶段使用。
