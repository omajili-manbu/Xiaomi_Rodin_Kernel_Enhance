# Xiaomi Rodin Kernel Enhance

[![Kernel](https://img.shields.io/badge/kernel-6.6.x_LTS-blue)](https://kernel.org)
[![Base](https://img.shields.io/badge/base-MiCode%20bsp--rodin--v--oss-orange)](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss)
[![Root](https://img.shields.io/badge/Root-SukiSU--Ultra-green)](https://github.com/SukiSU-Ultra/SukiSU-Ultra)
[![SuSFS](https://img.shields.io/badge/Hiding-SuSFS-green)](https://gitlab.com/simonpunk/susfs4ksu)
[![License](https://img.shields.io/badge/license-GPL--2.0-lightgrey)](LICENSES)

[English](README.en.md) | 简体中文

基于小米已开源的 [`bsp-rodin-v-oss`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss) 内核源码（AOSP `android15-6.6` GKI / 联发科）构建的 rodin 设备增强内核，并持续跟进最新 **6.6 LTS**。

## 特性

### Root 与隐藏
- 内置 **SukiSU-Ultra**（`drivers/kernelsu`），由 CI 自动追踪并合并上游 stable release tag（见下方"自动化"）
- 内置 **SuSFS**，集成自 [`omajili-manbu/susfs4ksu`（`gki-android15-6.6-mod`）](https://github.com/omajili-manbu/susfs4ksu/tree/gki-android15-6.6-mod)
- 具备对应用隐藏可疑 SELinux 上下文/规则的能力，绑定在 SuSFS AVC 日志欺骗开关

### 防格机
- **Baseband-guard (BBG)** LSM：在内核层拦截对关键分区/设备节点的未授权写入（来自 [vc-teahouse/Baseband-guard](https://github.com/vc-teahouse/Baseband-guard)，白名单已针对 rodin 调整）

### 性能
- 针对 Cortex-A725 的编译优化（clang 19+）
- **ThinLTO** 链接优化
- **AutoFDO** 基于实际运行场景的链接优化（clang 17+）
- **BBRv3** 作为默认 TCP 拥塞控制，**fq** 作为配套网络队列调度
- **ZSTD** 升级至 v1.5.7
- ZRAM 内建，压缩算法补全，默认 LZ4

### 稳定性修复
- 修复概率性开机卡死，恢复 vendor 模块兼容
- 在小米官方内核基础上反向移植上游修复

调优理念是**性能与续航兼顾**——所有提升均来自编译时优化、更新的内核、更新的算法，以及对小米官方内核的 bug 修复，没有偏向任何一方的激进调整。

## 自动化

- 每日自动合并 AOSP `android15-6.6-lts` 上游到被跟踪的 BSP 分支（矩阵覆盖 `bsp-rodin-v-upstream` 与 `bsp-rodin-v-upstream-noroot`）
- 每日追踪 [SukiSU-Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra) 的 stable release tag 并自动合并

## 支持设备

| 设备 | 代号 | 系统 |
|---|---|---|
| Redmi Turbo 4 | rodin | HyperOS 3（Android 16） |
| POCO X7 Pro | rodin | HyperOS 3（Android 16） |

> **说明：** 未在 POCO 实机上测试过，但大概率可用——小米在 POCO 与 Redmi 机型上使用同一套内核源码。遇到问题欢迎带日志开 issue，我会尝试修复。

## 分支

- [`bsp-rodin-v-upstream`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-v-upstream) — 本分支，主构建分支（内置 SukiSU-Ultra 与 SuSFS），Releases 由此创建
- [`bsp-rodin-v-upstream-noroot`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-v-upstream-noroot) — 本分支的无 Root 变体，构建产物仅提供 AnyKernel3 可刷包
- [`bsp-rodin-c-rebase`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-c-rebase) — 6.18 内核基线重建（进行中，详见该分支 README）
- [`bsp-rodin-c-port`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-c-port) — 在 6.18 内核上兼容 6.6 vendor 模块的实验分支（已冻结）
- [`c-rebase-16k`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/c-rebase-16k) — 6.18 树的 16K 页尺寸实验分支（已冻结）
- [`rodin-device-special`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/rodin-device-special) — rodin 设备定制文件
- [`rodin-lz4-v1.10.0-backport`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/rodin-lz4-v1.10.0-backport) — LZ4 v1.10.0 回移植（暂停更新）
- [`dash-w-oss`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/dash-w-oss) — REDMI Turbo 5 Max（Android W） 小米功能定制更新的参考内核

## 计划

- 适配 HyperOS 4

## 下载与支持

- [一些构建经验](BUILD-GUIDE.md)
- 预编译镜像：[Releases](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/releases)
- Releases 页面 tag 版本规则：例行更新会在版本号第三位累加，功能改进会直接提升第二位，比如 v1.2.6 » v1.3
- 问题反馈：请携带内核日志开 [issue](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/issues)
- 如果你觉得这个项目不错，欢迎点个 Star 支持我！

## 致谢

- [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) — rodin 官方内核源码（`bsp-rodin-v-oss`）
- [SukiSU-Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra) / [KernelSU](https://github.com/tiann/KernelSU) — Root 方案
- [SuSFS](https://gitlab.com/simonpunk/susfs4ksu) — Root 隐藏
- [Baseband-guard](https://github.com/vc-teahouse/Baseband-guard) — 防格机
- [Linux-Patches](https://gitlab.com/xanmod/linux-patches) — BBRv3 补丁
- AOSP `android15-6.6` / 上游 Linux 6.6 LTS

## 许可证

本仓库遵循 **GPL-2.0**，与 Linux 内核及其上游源码保持一致，详见 [LICENSES](LICENSES)。
