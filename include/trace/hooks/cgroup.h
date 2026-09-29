/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM cgroup
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH trace/hooks
#if !defined(_TRACE_HOOK_CGROUP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_CGROUP_H
#include <trace/hooks/vendor_hooks.h>

struct cgroup_taskset;
struct cgroup_subsys;
struct cgroup_subsys_state;

/*
 * rodin 6.9 (批4-9 hook ABI 审计) — **刻意与 AOSP 上游分叉，换 LTS 基线时必须重查**：
 * 6.6 ABI 保留为 2 参。消费者全部按 2 参编译：
 *   (a) 永久闭源 blob metis.ko 的 probe_android_vh_cgroup_set_task(@0x13c60) 反汇编
 *       `mov w20,w1 / mov x19,x2 / cbz x19 / ldr x8,[x19,#0x9f8]` ⇒ 第 2 个 TP 参被当
 *       task_struct 解引用。若内核传 4 参，metis 拿到的是 cgroup* ⇒ **运行期野指针**
 *       （不报编译错、装载也不报错，只在运行时偶发崩溃/状态错乱）；
 *   (b) 本批内建的 task_turbo.c handler 也不用 cgrp/threadgroup。
 * AOSP fc45b70eda9b (2025-02-11) 才把同名钩子扩成 4 参，与 6.6 blob 不兼容。
 * 回归手段：见 6.18-批4-9-hookABI审计-20260929.md §5.6（换基线后重跑 70_/90_ 脚本）。
 */
DECLARE_HOOK(android_vh_cgroup_set_task,
	TP_PROTO(int ret, struct task_struct *task),
	TP_ARGS(ret, task));

DECLARE_HOOK(android_vh_cgroup_attach,
	TP_PROTO(struct cgroup_subsys *ss, struct cgroup_taskset *tset),
	TP_ARGS(ss, tset));

DECLARE_RESTRICTED_HOOK(android_rvh_cgroup_force_kthread_migration,
	TP_PROTO(struct task_struct *tsk, struct cgroup *dst_cgrp, bool *force_migration),
	TP_ARGS(tsk, dst_cgrp, force_migration), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_cpuset_fork,
	TP_PROTO(struct task_struct *p, bool *inherit_cpus),
	TP_ARGS(p, inherit_cpus), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_cpu_cgroup_attach,
	TP_PROTO(struct cgroup_taskset *tset),
	TP_ARGS(tset), 1);

DECLARE_RESTRICTED_HOOK(android_rvh_cpu_cgroup_online,
	TP_PROTO(struct cgroup_subsys_state *css),
	TP_ARGS(css), 1);

DECLARE_HOOK(android_vh_cpuset_attach_task,
	TP_PROTO(struct cgroup_subsys_state *css, struct task_struct *task),
	TP_ARGS(css, task));

DECLARE_HOOK(android_vh_cpuset_css_online,
	TP_PROTO(struct cgroup_subsys_state *css),
	TP_ARGS(css));
#endif

#include <trace/define_trace.h>
