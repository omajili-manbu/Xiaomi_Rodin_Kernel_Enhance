# Xiaomi Rodin Kernel — 6.18 Rebase

[![Kernel](https://img.shields.io/badge/kernel-6.18.x_LTS-blue)](https://kernel.org)
[![Base](https://img.shields.io/badge/base-MiCode%20bsp--rodin--v--oss-orange)](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss)
[![License](https://img.shields.io/badge/license-GPL--2.0-lightgrey)](LICENSES)

本分支（`bsp-rodin-c-rebase`）把 rodin 设备（Redmi Turbo 4 / POCO X7 Pro，联发科 mt6899 平台）的内核支持从小米官方 [`bsp-rodin-v-oss`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/bsp-rodin-v-oss)（6.6）整体重建到 **6.18 LTS**，基线为 AOSP common kernel `android17-6.18-lts`（当前 6.18.33）。

6.6 稳定线的特性、下载与使用说明见主分支 [README](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-v-upstream)；本文件只覆盖 6.18 重建树。

## 方案概要

与 6.6 稳定线沿用 stock `vendor_dlkm` 模块形态不同，6.18 线把厂商代码**全量内建（=y）**编进内核镜像：

- 以 stock `vendor_dlkm` 的 569 个唯一模块为对账基线：539 个已以内建形式编入内核——其中 10 个闭源 blob 单元以机器码重建胶水接入；其余 30 个经审计无运行期消费者，不再装载。内核 `modules.builtin` 内建清单共 1208 项。
- 6.18 相对 6.6 的 API / 行为差异逐件适配，落码记录在两个 vendor 仓的独立提交里。

## 树结构

| 路径 | 说明 |
|---|---|
| 内核源码（树根） | AOSP `android17-6.18-lts` 基线 + rodin 设备补丁 |
| `MTK_kernel_device_modules-rodin/` | 设备模块 vendor 仓（独立 git 仓，分支 `bsp-rodin-v-oss`），6.18 化修改的主要落点 |
| `MTK_kernel_modules-rodin/` | 技术模块 vendor 仓（独立 git 仓，分支 `bsp-rodin-v-oss`；mali / conninfra 等） |
| `rodin-ext/` | 接线层：`Kconfig.ext` / `Makefile.ext` + 24 个相对 symlink 镜像，把两棵 vendor 树挂进内核 Kconfig 与 obj-y |
| `GNUmakefile` | 构建入口：固化 vendor 接线 5 变量与 `MTK_PLATFORM=mt6899`、工具链路径、`ARCH=arm64 LLVM=1` |
| `dwarves/` | pahole v1.32 历史遗迹（BTF 实际走 `tools/pahole`，见 `Makefile:558`，不经 PATH） |
| `tools/` | 构建 / 审计 / 刷机脚本 |

工具链 prebuilts（clang-r596125、Rust 1.97.0）与 6.6 构建树共用，放在本树上级 `prebuilts/` 目录，不在树内。

## 构建

在树根直接执行：

```bash
make
```

- 缺 `.config` 时自动 `gki_defconfig` + `olddefconfig` 再构建；config 只走 `gki_defconfig`。
- **不要** `make -f Makefile …`：会绕过 `GNUmakefile` 固化的接线变量，vendor Kconfig 符号宇宙为空，构建"成功"但产物错误。`O=` 外部输出目录同样不支持（树内构建）。
- Rust 必须保持开启：6.18 的 ashmem 是 Rust 实现，`RUST` 不开时 `CONFIG_ASHMEM` 会被静默丢弃，stock HyperOS 用户态因拿不到 `/dev/ashmem` 而启动循环。
- 产物：`Image.lz4`（约 44 MB，上限 60 MB）与 `modules.builtin`。`Image.lz4` 为 lz4 legacy 格式，刷机时打入 `boot.img`。

## 相关分支

- [`bsp-rodin-c-rebase`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-c-rebase) — 本分支，6.18 主线（4K 页）
- [`bsp-rodin-c-port`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/bsp-rodin-c-port) — 在 6.18 内核上直接兼容 6.6 vendor 模块的对照实验
- [`c-rebase-16k`](https://github.com/omajili-manbu/Xiaomi_Rodin_Kernel_Enhance/tree/c-rebase-16k) — 16K 页尺寸实验分支（已冻结）

## 许可证

GPL-2.0，与 Linux 内核一致，详见 [LICENSES](LICENSES)。
