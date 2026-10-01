/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VSEQ_H
#define _LINUX_VSEQ_H
/*
 * VSEQ —— 原厂 first-stage 模块装载时序的内核内重放（rodin 第二十四轮）
 *
 * 背景：6.6 原厂 220+ 个 vendor 驱动是 .ko，由 Android init 的 libmodprobe
 * 按 vendor_boot 的 modules.load 行序 × modules.dep 硬依赖拓扑装载；
 * =y 化后全部退化为 initcall（层级+链接序），三条原厂时序契约
 * （文件系统/提供者就绪/晚装载豁免）全部断裂，是前二十三轮真机雷的主根因。
 *
 * 本机制：被批改为 vseq_* 宏的原 .ko 单元，其 module_init 族不再进入标准
 * initcall 层级，而是落入 .vseq.entries 段；do_initcalls() 全部跑完后由
 * vseq_replay() 按 drivers/base/vseq_order_gen.c 的顺序表（= 6.6 真机
 * 实测 224 条装载序，tools/_b523_vseq/gen_vseq.py 对账生成）逐个同步调用，
 * 等价于内核自己当一次 init。__init 文本此时仍存活（free_initmem 之前），
 * 与原厂 0.41s insmod 波同语义。
 *
 * 语义边界：只有"原厂就是 .ko"的单元进 VSEQ；原厂内建驱动保持原 initcall
 * 层级不动。skip 表模块（无源/blob，仍走 .ko 装载）不在 VSEQ 集合内。
 */
#include <linux/init.h>
#include <linux/types.h>

struct vseq_ent {
	int (*fn)(void);
	const char *mod;
};

struct vseq_mod_order {
	const char *name;	/* 归一化模块名（-/ _ 等价，小写） */
	unsigned int seq;	/* 原厂装载序号 */
};

extern const struct vseq_mod_order __vseq_mod_order[];
extern const unsigned int __vseq_mod_order_nr;

#ifdef CONFIG_DEVICE_MODULES_ALLOW_BUILTIN
void vseq_replay(void);
#else
static inline void vseq_replay(void) {}
#endif

#define __vseq_entry(__fn, __mod) \
	static const struct vseq_ent __vseq_ent_##__fn __used \
	__section(".vseq.entries") = { .fn = (__fn), .mod = (__mod) };

/*
 * 全部别名同义展开：.ko 语境下 *_initcall 宏本就折叠为 module_init，
 * 1:1 别名只为保留代码形态可检索。
 */
#define vseq_module_init(fn)		__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_core_initcall(fn)		__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_early_initcall(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_postcore_initcall(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_arch_initcall(fn)		__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_subsys_initcall(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_fs_initcall(fn)		__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_rootfs_initcall(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_device_initcall(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_late_initcall(fn)		__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_subsys_initcall_sync(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_arch_initcall_sync(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_fs_initcall_sync(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_rootfs_initcall_sync(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_device_initcall_sync(fn)	__vseq_entry(fn, KBUILD_MODNAME)
#define vseq_late_initcall_sync(fn)	__vseq_entry(fn, KBUILD_MODNAME)

/*
 * 驱动注册宏族：与 include/linux 同名宏逐一等价，仅 module_init →
 * vseq_module_init（module_exit 原样保留，=y 下本就编译为空）。
 */
#define vseq_module_driver(__driver, __register, __unregister, ...) \
static int __init __driver##_init(void) \
{ \
	return __register(&(__driver) , ##__VA_ARGS__); \
} \
vseq_module_init(__driver##_init); \
static void __exit __driver##_exit(void) \
{ \
	__unregister(&(__driver) , ##__VA_ARGS__); \
} \
module_exit(__driver##_exit);

#define vseq_module_platform_driver(__platform_driver) \
	vseq_module_driver(__platform_driver, platform_driver_register, \
			platform_driver_unregister)

#define vseq_module_platform_driver_probe(__platform_driver, __platform_probe) \
static int __init __platform_driver##_init(void) \
{ \
	return platform_driver_probe(&(__platform_driver), \
				     __platform_probe);    \
} \
vseq_module_init(__platform_driver##_init); \
static void __exit __platform_driver##_exit(void) \
{ \
	platform_driver_unregister(&(__platform_driver)); \
} \
module_exit(__platform_driver##_exit);

#define vseq_module_i2c_driver(__i2c_driver) \
	vseq_module_driver(__i2c_driver, i2c_add_driver, \
			i2c_del_driver)

#define vseq_module_mipi_dsi_driver(__mipi_dsi_driver) \
	vseq_module_driver(__mipi_dsi_driver, mipi_dsi_driver_register, \
			mipi_dsi_driver_unregister)

#define vseq_module_spmi_driver(__spmi_driver) \
	vseq_module_driver(__spmi_driver, spmi_driver_register, \
			spmi_driver_unregister)

#define vseq_module_scmi_driver(__scmi_driver)	\
	vseq_module_driver(__scmi_driver, scmi_register, scmi_unregister)

#endif /* _LINUX_VSEQ_H */
