# Xiaomi Rodin Kernel Enhance

[![Kernel](https://img.shields.io/badge/kernel-6.6.142_LTS-blue)](https://kernel.org)
[![Base](https://img.shields.io/badge/base-MiCode%20bsp--rodin--v--oss-orange)](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss)
[![Root](https://img.shields.io/badge/Root-ReSukiSU-green)](https://github.com/ReSukiSU/ReSukiSU)
[![SuSFS](https://img.shields.io/badge/Hiding-SuSFS-green)](https://gitlab.com/simonpunk/susfs4ksu)
[![License](https://img.shields.io/badge/license-GPL--2.0-lightgrey)](LICENSES)

English | [简体中文](README.zh-CN.md)

An enhanced Android kernel for Xiaomi **rodin** devices, built on top of Xiaomi's released [`bsp-rodin-v-oss`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss) kernel source (AOSP `android15-6.6` GKI / MediaTek) and kept in sync with the latest **6.6 LTS**.

## Highlights

### Root & Hiding
- **ReSukiSU** built-in, integrated from [`omajili-manbu/ReSukiSU` (`main-susfs`)](https://github.com/omajili-manbu/ReSukiSU/tree/main-susfs)
- **SuSFS** with upstream sync, integrated from [`omajili-manbu/susfs4ksu` (`gki-android15-6.6-mod`)](https://github.com/omajili-manbu/susfs4ksu/tree/gki-android15-6.6-mod)
- Hiding of suspicious SELinux contexts/rules from app-visible policy probes, tied to the SuSFS AVC log spoofing switch

### Brick Protection
- **Baseband-guard (BBG)** LSM: blocks unauthorized writes to critical partitions/device nodes at the kernel level ([vc-teahouse/Baseband-guard](https://github.com/vc-teahouse/Baseband-guard), allowlist adjusted for rodin)

### Performance
- Cortex-A725 compiler tuning (requires clang 19+) + **ThinLTO**
- **BBR** as default TCP congestion control, **fq** qdisc
- **ZSTD** upgraded to v1.5.7
- ZRAM built-in with a full compression algorithm set, default LZ4

### Stability & Fixes
- Fixed probabilistic boot hang and restored vendor module compatibility
- Backported upstream fixes on top of Xiaomi's official kernel
- Unprivileged BPF disabled by default

The tuning philosophy is **balanced** — all gains come from compile-time optimizations, an up-to-date kernel, updated compression algorithms and bug fixes on top of Xiaomi's official kernel, with nothing biased toward either performance or battery life.

## Supported Devices

| Device | Codename | OS |
|---|---|---|
| Redmi Turbo 4 | rodin | HyperOS 3 (Android 16) |
| POCO X7 Pro | rodin | HyperOS 3 (Android 16) |

> **Note:** Untested on POCO hardware, but it should work — Xiaomi ships the same kernel source for the POCO and Redmi variants. If you run into any issues, feel free to open an issue with logs attached, and I'll try to fix it.

## Branches

- [`bsp-rodin-v-oss-bp`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-v-oss-bp) — main build branch, based on Xiaomi's official rodin source with additional backports and enhancements
- Release tags (`v1.x`) mark the exact commit of each prebuilt release

## Roadmap

- HyperOS 4 support is planned

## Downloads & Support

- Prebuilt images: [Releases](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/releases)
- Bug reports: open an [issue](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/issues) with kernel logs attached

## Acknowledgements

- [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) — official rodin kernel source (`bsp-rodin-v-oss`)
- [ReSukiSU](https://github.com/ReSukiSU/ReSukiSU) / [SukiSU-Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra) / [KernelSU](https://github.com/tiann/KernelSU) — root solution
- [SusFS](https://gitlab.com/simonpunk/susfs4ksu) — root hiding
- [Baseband-guard](https://github.com/vc-teahouse/Baseband-guard) — brick protection
- AOSP `android15-6.6` / upstream Linux 6.6 LTS

## License

This repository is licensed under **GPL-2.0**, following the Linux kernel and the upstream sources it is based on. See [LICENSES](LICENSES) for details.
