# 扩展性功能清理记录

2026-11-24。复查工作区和 Git 忽略目录，仅清理双指触控、桌面静默启动之后新增或重新生成的构建产物。上一轮范围沿用[日常使用清理记录](daily-use-cleanup.md)。

| 范围 | 文件数 | 字节 | 重建入口 |
| --- | ---: | ---: | --- |
| 鸿蒙 Hvigor 缓存和 `harmony/entry/build/default/` 下的缓存及中间产物 | 67 | 7,527,519 | 按 README 使用 DevEco Studio 26.0.0、SDK API 26 和现有锁定依赖 |
| 两个 Windows 发送端临时构建对象及两个触控测试对象 | 4 | 4,333,779 | `pc/build.ps1`、`pc/tests/touch_frames.cpp` 和本地测试窗口源码，使用 MSVC 14.44.35207、Windows SDK 10.0.26100.0 |
| 合计 | 71 | 11,861,298 | |

逐项核对项目内绝对路径、重解析点、Git 忽略状态、大小和 SHA-256 后删除。复点由 2,481 个文件、174,967,831 字节降至 2,410 个文件、163,106,533 字节，排除根目录 `.git` 和重解析点，以写入清单后为测量起点。逐文件清单位于 `.local/evidence/2026-11-24/extended-features-cleanup-files.json`，SHA-256 为 `6088e06fbaacc48a1453f57f550a02f24992c788f0f1db0ea98d9916e23f8b82`。相关忽略规则已存在。

原始手势、失败诊断、应用退出和拔线日志保留于 `.local/daily-tests/`；本地验证脚本、构建源码快照、回滚包和旧发送端保留于 `.local/multi-touch-build/` 和 `.local/desktop-session-build/`。当前发送端、签名 HAP、HDC 启动脚本和构建报告仍在原位。迁移时按原相对路径一并携带这些 Git 忽略项。清理过程没有重新启动真机测试。[USB 验证记录](usb-verification.md)
