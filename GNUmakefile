# GNUmakefile — rodin 6.18 固化构建变量包装层（v2，2026-09-26 归拢后生效）
#
# 用法：在本树根目录直接 `make` 即可（缺 .config 时自动 gki_defconfig+olddefconfig
#       再构建；make gki_defconfig / clean 等照常可用）。
#
# 为什么必须有本文件：树根 0 字节 Makefile.ext / 占位 Kconfig.ext 是"无变量静默
# 空转"的兜底——不带下面 5 个接线变量跑 defconfig，vendor Kconfig 符号宇宙为空、
# gki_defconfig 里的键全部被丢、构建照样成功但产物不对（2026-09-25 事故先例）。
# 本文件把变量固化进 make，杜绝该环境态陷阱。make 会自动选中本文件
# （GNUmakefile > makefile > Makefile）；不要 `make -f Makefile ...` 绕过它。
# 优先级：命令行 > 本文件 > 环境。
#
# O= 外部输出目录不支持（r25 起为树内构建；O= 的重入会绕过本文件）。
#
# 变量值 = _r25fix17_fullbuild.sh 原样（vendor 树已物理迁入树内，../ 一层减掉）：
#   Kconfig:37            source "$(KCONFIG_EXT_PREFIX)Kconfig.ext"
#   drivers/Makefile:203  include $(srctree)/$(KCONFIG_EXT_PREFIX)Makefile.ext
#   rodin-ext/Makefile.ext: obj-y += ../$(DEVICE_MODULES_REL_DIR)/drivers/...
#                         （../ 从 drivers/ 起一跳到树根；vendor 树就挂在树根）
#   rodin-ext/drivers/*   24 个相对 symlink 镜像，仅供 Kconfig source 解析

RODIN_TREE := $(CURDIR)

# ---- 工具链（= _r25fix17_fullbuild.sh 原样：dwarves 的 pahole v1.32 + clang-r596125） ----
# clang 工具链不放树内（2026-09-26 定案）：prebuilts（clang/pahole/rust）留在
# rodin-build 顶层与 bp 树共用，本文件以 $(RODIN_TREE)/../prebuilts 引用；
# 树内只保留 dwarves。
# rust：2026-10-12 起启用（目的 = 恢复 /dev/ashmem：6.18 的 ashmem 是 Rust 实现
# （drivers/staging/android/ashmem.rs，Kconfig `depends on SHMEM && RUST`），RUST 不开时
# CONFIG_ASHMEM 被静默丢弃，stock HyperOS 的 system_server 会在 ApplicationSharedMemory
# 上 FATAL 崩、启动循环）。
# 旧注记里"编不过"的两条实锤属兼容层树（该树路径历史上曾用本名）：本树无 *_k618 改名、
# drm_driver 亦无 lastclose/date 槽位，两处修法均不适用（2026-10-12 grep 复核）。
# 工具链 = ../prebuilts/rust-toolchain/linux-x86/1.97.0（rustc 的 LLVM 22.1.6 与
# clang-r596125 的 22.0.2 同代）；LIBCLANG_PATH 必须显式给（r24 教训：不设会落到
# 系统 libclang 18）。
# 副作用（刷机判读用）：CONFIG_RUST select EXTENDED_MODVERSIONS + CFI_ICALL_NORMALIZE_INTEGERS，
# 相对出厂 6.6 是规则面净变化（见 tools/_b566_bootfix/apply_rust_route_b566.py 的 ABI 门禁说明）。
RODIN_CLANG_DIR := clang-r596125
RODIN_RUST_DIR := rust-toolchain/linux-x86/1.97.0
export PATH := $(RODIN_TREE)/dwarves:$(RODIN_TREE)/../prebuilts/$(RODIN_RUST_DIR)/bin:$(RODIN_TREE)/../prebuilts/clang/host/linux-x86/$(RODIN_CLANG_DIR)/bin:$(PATH)
export LIBCLANG_PATH := $(RODIN_TREE)/../prebuilts/clang/host/linux-x86/$(RODIN_CLANG_DIR)/lib

# ---- KCFLAGS：rust bitcode 编译步的旗标冲突兜底（2026-10-12） ----
# rust/Makefile:469（inline-helpers）用 clang 编 *.m.bc，吃整个 $(KBUILD_CFLAGS)：
# 其中既有内核默认 -Werror=unused-command-line-argument，又有 vendor 的全局 -I
# （C 面必需；bitcode 输入用不到 -I）⇒ clang 把 "argument unused" 抬成 error，
# drivers/staging/android/ashmem.o 编不出来（CONFIG_ASHMEM=y 的唯一阻塞点）。
# Makefile:1140 `KBUILD_CFLAGS += $(KCFLAGS)` 追加在末尾 ⇒ 后置 -Wno 覆盖前置 -Werror。
# 只影响诊断、不改代码生成；C 编译面不受影响（-I 在 C 里不算 unused）。
export KCFLAGS := -Wno-unused-command-line-argument

# ---- 架构 ----
export ARCH := arm64
export LLVM := 1

# ---- vendor 外挂接线 5 变量（r25 语义，树内布局） ----
# KCONFIG_EXT_PREFIX 必须相对 srctree 且带尾斜杠（两处钩子都是前缀拼接）。
export KCONFIG_EXT_PREFIX := rodin-ext/
# obj 路径：drivers/ 下 ../<名> = 树根/<名>。外部时代靠 Makefile.ext 的
# KMOD_REL_DIR ?= ../MTK_kernel_modules-rodin 兜底，入树后必须显式去掉 ../。
export DEVICE_MODULES_REL_DIR := MTK_kernel_device_modules-rodin
export KMOD_REL_DIR := MTK_kernel_modules-rodin
# 这两个必须用 := 简单变量 + $(CURDIR) 绝对路径，不能用 = $(srctree)/... 递归式：
# export 出去的递归变量在首层 make 构造子进程环境时就会被展开，而那一刻内核
# Makefile 还没定义 srctree ⇒ 环境里落成 "/MTK_..."（指向文件系统根），vendor
# 的 -I（含 timer-mediatek-trace.h 的 trace 重包含路径）全部失联（mig5 实测）。
# r25 命令行写法能活是因为命令行变量经 MAKEFLAGS 逐层展开。绝对路径仅影响
# .cmd 里的字面量，不影响产物。
export DEVICE_MODULES_PATH := $(CURDIR)/MTK_kernel_device_modules-rodin
# MTK 自己的拼写错误（DEVCIE_），照 _r25fix17 原样保留。
export DEVCIE_MODULES_INCLUDE := -I$(CURDIR)/MTK_kernel_device_modules-rodin/include
export MTK_PLATFORM := mt6899

# ---- 默认目标：必须包一层 ----
# 内核 Makefile 全部 body 位于 need-sub-make 的 else 分支；.DEFAULT_GOAL 直接指
# Image.lz4 时首层 make 因缺 --no-print-directory 走重入分支、body 未读，
# Image.lz4 无规则直接死（"No rule to make target 'Image.lz4'"）。
# 包一层变显式目标后与 `make Image.lz4` 走完全相同的重入机制。
ifeq ($(wildcard $(CURDIR)/.config),)
.PHONY: rodin-config
rodin-config:
	$(MAKE) gki_defconfig
	$(MAKE) olddefconfig
rodin-default: rodin-config
endif

.PHONY: rodin-default
rodin-default:
	+$(MAKE) Image.lz4

.DEFAULT_GOAL := rodin-default

include Makefile
