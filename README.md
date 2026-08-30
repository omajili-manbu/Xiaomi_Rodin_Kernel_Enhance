# Xiaomi Rodin Kernel Enhance

[![Kernel](https://img.shields.io/badge/kernel-6.6.142_LTS-blue)](https://kernel.org)
[![Base](https://img.shields.io/badge/base-MiCode%20bsp--rodin--v--oss-orange)](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss)
[![Root](https://img.shields.io/badge/Root-ReSukiSU-green)](https://github.com/ReSukiSU/ReSukiSU)
[![SuSFS](https://img.shields.io/badge/Hiding-SuSFS-green)](https://gitlab.com/simonpunk/susfs4ksu)
[![License](https://img.shields.io/badge/license-GPL--2.0-lightgrey)](LICENSES)

**English** | [简体中文](#简体中文)

An enhanced Android kernel for Xiaomi **rodin** devices, built on top of Xiaomi's released [`bsp-rodin-v-oss`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss) kernel source (Android15-6.6 GKI / MediaTek) and kept in sync with the latest **6.6 LTS**.

## Highlights

### Root & Hiding
- **ReSukiSU** built-in (`drivers/kernelsu`, integrated via the `resukisu-susfs` tree)
- **SuSFS** with upstream sync (`TIF_PROC_NO_SU` + `zygote_next`)
- Hiding of suspicious SELinux contexts/rules from app-visible policy probes, tied to the SuSFS AVC log spoofing switch

### Brick Protection
- **Baseband-guard (BBG)** LSM: blocks unauthorized writes to critical partitions/device nodes at the kernel level ([vc-teahouse/Baseband-guard](https://github.com/vc-teahouse/Baseband-guard), allowlist adjusted for rodin)

### Performance
- Cortex-A725 compiler tuning (`-mcpu/-mtune`, requires clang 19+) + **ThinLTO**
- **BBR** as default TCP congestion control (BIC/CUBIC/WESTWOOD/HTCP also available), **fq** qdisc
- **ZSTD** upgraded to v1.5.7
- ZRAM built-in with a full compression algorithm set (LZ4 / LZ4HC / DEFLATE / 842 / ZSTD), default LZ4

### Stability & Fixes
- Fixed probabilistic boot hang (`init` SIGILL) and restored vendor module compatibility (6.6.77 vendor modules, `zsmalloc.ko` symbol/CRC issues)
- Backported fixes: fscrypt, `mm/swap_cgroup` NULL deref, BPF `lpm_trie` UBSAN OOB
- Unprivileged BPF disabled by default (`CONFIG_BPF_UNPRIV_DEFAULT_OFF=y`)

The tuning philosophy is **balanced** — all gains come from compile-time optimizations, an up-to-date kernel, updated compression algorithms and bug fixes on top of Xiaomi's official kernel, with nothing biased toward either performance or battery life.

## Supported Devices

| Device | Codename | OS |
|---|---|---|
| Redmi Turbo 4 | rodin | HyperOS 3 (Android 15) |
| POCO X7 Pro | rodin | HyperOS 3 (Android 15) |

> **Note:** Untested on POCO hardware, but it should work — Xiaomi ships the same kernel source for the POCO and Redmi variants. If you run into any issues, feel free to open an issue with logs attached, and I'll try to fix it.

## Branches

- [`bsp-rodin-v-oss-bp`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-v-oss-bp) — main build branch, based on Xiaomi's official rodin source with additional backports and enhancements
- Release tags (`v1.x`) mark the exact commit of each prebuilt release

## Roadmap

- HyperOS 4 support is planned

## Downloads & Support

- Prebuilt images: [Releases](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/releases)
- Flashing notes and announcements: [Discussions](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/discussions)
- After flashing, `uname -r` should report something like `6.6.142-by-Omachirimanbu-4k-g<commit>`
- Bug reports: open an [issue](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/issues) with kernel logs (`dmesg` / `last_kmsg`) attached

## Acknowledgements

- [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) — official rodin kernel source (`bsp-rodin-v-oss`)
- [ReSukiSU](https://github.com/ReSukiSU/ReSukiSU) / [SukiSU-Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra) / [KernelSU](https://github.com/tiann/KernelSU) — root solution
- [SusFS](https://gitlab.com/simonpunk/susfs4ksu) — root hiding
- [Baseband-guard](https://github.com/vc-teahouse/Baseband-guard) — brick protection
- AOSP `android15-6.6` / upstream Linux 6.6 LTS

## License

This repository is licensed under **GPL-2.0**, following the Linux kernel and the upstream sources it is based on. See [LICENSES](LICENSES) for details.

---

# 简体中文

[English](#xiaomi-rodin-kernel-enhance) | 简体中文

基于小米已开源的 [`bsp-rodin-v-oss`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss) 内核源码（Android15-6.6 GKI / 联发科）构建的 rodin 设备增强内核，并持续跟进最新 **6.6 LTS**。

## 特性

### Root 与隐藏
- 内置 **ReSukiSU**（`drivers/kernelsu`，经由 `resukisu-susfs` 树集成）
- **SuSFS**，已跟进上游（`TIF_PROC_NO_SU` + `zygote_next`）
- 对应用隐藏可疑 SELinux 上下文/规则，绑定到 SuSFS AVC 日志欺骗开关

### 防格机
- **Baseband-guard (BBG)** LSM：在内核层拦截对关键分区/设备节点的未授权写入（来自 [vc-teahouse/Baseband-guard](https://github.com/vc-teahouse/Baseband-guard)，白名单已针对 rodin 调整）

### 性能
- 针对 Cortex-A725 的编译优化（`-mcpu/-mtune`，需要 clang 19+）+ **ThinLTO**
- **BBR** 作为默认 TCP 拥塞控制（同时可用 BIC/CUBIC/WESTWOOD/HTCP），网络队列调度 **fq**
- **ZSTD** 升级至 v1.5.7
- ZRAM 内建，压缩算法补全（LZ4 / LZ4HC / DEFLATE / 842 / ZSTD），默认 LZ4

### 稳定性修复
- 修复概率性开机卡死（`init` SIGILL），恢复 vendor 模块兼容（6.6.77 vendor 模块、`zsmalloc.ko` 符号/CRC 问题）
- 反向移植修复：fscrypt、`mm/swap_cgroup` 空指针、BPF `lpm_trie` UBSAN 数组越界
- 默认禁用非特权 BPF（`CONFIG_BPF_UNPRIV_DEFAULT_OFF=y`）

调优方向是**性能与续航兼顾**——所有提升均来自编译时优化、更新的内核、更新的压缩算法，以及对小米官方内核的 bug 修复，没有偏向任何一方的激进调整。

## 支持设备

| 设备 | 代号 | 系统 |
|---|---|---|
| Redmi Turbo 4 | rodin | HyperOS 3（Android 15） |
| POCO X7 Pro | rodin | HyperOS 3（Android 15） |

> **说明：** 未在 POCO 实机上测试过，但大概率可用——小米在 POCO 与 Redmi 机型上使用同一套内核源码。遇到问题欢迎带日志开 issue，我会尝试修复。

## 分支

- [`bsp-rodin-v-oss-bp`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-v-oss-bp) — 主构建分支，在小米官方 rodin 源码基础上叠加反向移植与增强
- Release 标签（`v1.x`）标记每个预编译版本对应的提交

## 计划

- 适配 HyperOS 4

## 下载与支持

- 预编译镜像：[Releases](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/releases)
- 刷入说明与公告：[Discussions](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/discussions)
- 刷入后可用 `uname -r` 验证，应显示类似 `6.6.142-by-Omachirimanbu-4k-g<commit>`
- 问题反馈：请携带内核日志（`dmesg` / `last_kmsg`）开 [issue](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/issues)

## 致谢

- [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) — rodin 官方内核源码（`bsp-rodin-v-oss`）
- [ReSukiSU](https://github.com/ReSukiSU/ReSukiSU) / [SukiSU-Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra) / [KernelSU](https://github.com/tiann/KernelSU) — Root 方案
- [SusFS](https://gitlab.com/simonpunk/susfs4ksu) — Root 隐藏
- [Baseband-guard](https://github.com/vc-teahouse/Baseband-guard) — 防格机
- AOSP `android15-6.6` / 上游 Linux 6.6 LTS

## 许可证

本仓库遵循 **GPL-2.0**，与 Linux 内核及其上游源码保持一致，详见 [LICENSES](LICENSES)。
