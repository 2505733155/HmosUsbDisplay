# 日常使用阶段清理记录

2026-11-24。复查整个项目，包括 Git 忽略的构建、依赖、历史试验、签名输出和回滚目录。公开仓库准备阶段清理过的 15 项沿用原记录 `.local/evidence/2026-11-24/repo-prep-cleanup.json`。已封存的旧工程和上轮预检副本本次只清点占用。

| 清理范围 | 文件数（前 → 后） | 清理字节数 | 恢复入口 |
| --- | ---: | ---: | --- |
| `pc/subscreen_sender.obj` | 1 → 0 | 2,139,065 | `pc/build.ps1`，MSVC 14.44.35207 和 Windows SDK 10.0.26100.0 |
| `harmony/.hvigor/cache/` | 5 → 0 | 151,406 | 按 README 使用 DevEco Studio 26.0.0、SDK API 26 和锁定依赖重建 |
| `harmony/entry/build/default/cache/` | 26 → 0 | 393,335 | 同上 |
| `harmony/entry/build/default/intermediates/` | 36 → 0 | 6,929,679 | 同上 |
| 合计 | 68 → 0 | 9,613,485 | |

工作区首次盘点为 2,418 个文件、161,956,706 字节。删除后、编写本记录前复点为 2,351 个文件、152,381,115 字节。统计排除根目录 `.git` 与重解析点，保留运行日志在盘点期间增长，另有清理清单与文档变动，实际释放量按删除清单计算。

删除前逐项检查绝对路径、父目录重解析点、Git 忽略状态、大小和 SHA-256。删除后确认 68 项均已移除。逐文件凭据存于 `.local/evidence/2026-11-24/daily-use-cleanup-files.json`，SHA-256 为 `f33a908ab3ea5c702b04c2e04a0f254876ec1eacf7491e7f124c502f3e217249`。删除项均已有忽略规则。

离线检查通过。Windows 发送端在临时目录重建，保留原有 MSVC C4996 警告。单指协议检查通过，读完上游 ZIP 和两个 HAP 的全部文件成员，打开三张代表性证据图，并核对看板链接和 VS Code 启动入口。

## 保留与迁移

- 迁移或备份时一并携带 `.local/daily-tests/` 和 `.local/evidence/`，保留原相对路径。其中有原始日志、遥测、手势记录、截图、失败与未完成轮次、试验脚本和清理凭据。还应保留 `pc/sender.log`、`pc/crash_probe.log` 及现有构建日志和报告（`harmony/.hvigor/outputs/`、`harmony/.hvigor/report/`）。原始试验文件均保留展开副本。
- 携带 `.local/upstream/SubScreen-source.zip` 和 `.local/previous-layout/` 以保留上游基线和回滚工程。上游 ZIP 对应提交 `a070d8910e0b6ed79d202cd2adefb8b23ca70a23`，SHA-256 为 `118926a85fce5bef2c38fa305e2aac7e59389ee378dfd9e2efffe9de3435b3f1`。
- 立即试用所需的 `.local/start-subscreen.cmd`、`pc/subscreen_sender.exe` 和 `harmony/entry/build/default/outputs/default/` 保留在原处。迁移到新机器后更新启动脚本中的 HDC 路径。若复用旧签名，单独私密备份 `harmony/build-profile.json5` 和它引用的本机签名材料。
- 历史两次桌面卡死的日志在采集初始化阶段中止，根因仍未确认。按住时拔线与旧版接收端兼容仍缺专项实测。本批次只做离线核对，未运行硬件实验。

[USB 真机验证记录](usb-verification.md) · [日常使用排查参考](daily-use-reference.md)
