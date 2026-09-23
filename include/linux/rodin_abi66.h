/* SPDX-License-Identifier: GPL-2.0 */
/*
 * 6.6 ABI 兼容守卫：判别"厂商预编译模块提供的 ops 表"。
 *
 * 厂商模块按 6.6 布局静态构造 ops 表（表长 == 6.6 sizeof）。6.18 新增的字段对
 * 模块表而言落在表尾之外 —— 内核直接读会读到相邻 .rodata（常见是另一张表的首槽，
 * 即一个合法函数指针），然后以错误原型调用它，症状是难定位的野回调 / 写只读页。
 *
 * 故凡"6.18 新增字段"的读取点，先用本守卫：ops 位于模块地址空间 ⇒ 该能力不支持。
 * 这一族缺陷由 tools/ko_defdata_audit.py 通查（名字投票 + 内核可读 + 可达性），
 * 布局重排见 tools/apply_r13_vtables.py 与 tools/r13_vtables.spec。
 */
#ifndef _LINUX_RODIN_ABI66_H
#define _LINUX_RODIN_ABI66_H

#include <linux/module.h>

static inline bool rodin_66_module_ops(const void *ops)
{
	return ops && is_module_address((unsigned long)ops);
}

/* 6.18 新增字段的安全读取：模块表没有这个槽位 → 取 NULL（= 不支持该能力） */
#define RODIN_66_NEWFIELD(p, f) (rodin_66_module_ops(p) ? NULL : (p)->f)

/* 同上，但用于**位图/数组下标**场景：模块表取 0（= 不支持/无此项） */
#define RODIN_66_NEWIDX(p, f) (rodin_66_module_ops(p) ? 0 : (p)->f)

#endif /* _LINUX_RODIN_ABI66_H */
