/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM futex
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH trace/hooks
#if !defined(_TRACE_HOOK_FUTEX_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_FUTEX_H
#include <trace/hooks/vendor_hooks.h>
/*
 * Following tracepoints are not exported in tracefs and provide a
 * mechanism for vendor modules to hook and extend functionality
 */
struct plist_node;
struct plist_head;
DECLARE_HOOK(android_vh_alter_futex_plist_add,
	TP_PROTO(struct plist_node *node,
		 struct plist_head *head,
		 bool *already_on_hb),
	TP_ARGS(node, head, already_on_hb));

DECLARE_HOOK(android_vh_futex_sleep_start,
	TP_PROTO(struct task_struct *p),
	TP_ARGS(p));

/*
 * rodin 6.9 (批4-9 hook ABI 审计) — **刻意与 AOSP 上游分叉，换 LTS 基线时必须重查**：
 * 唯一消费者是永久闭源 blob metis.ko 的 mi_do_futex(@0x1ec8)，其反汇编为
 * `mov w19,w1 / cmp w19,#0x12 / b.hi(早退) / ldrsw [x9,x8,lsl#2]; br(按首参查跳表)`
 * ⇒ 编译期首参就是 `int cmd`。AOSP f97958c0be81 (2026-03-23) 把 uaddr 插到首位后，
 * metis 收到的"cmd"变成 uaddr 指针低 32 位 ⇒ 全部提前 return（钓子静默失效），
 * 且极小概率触发野指针。故这里保留 6.6 的 3 参形态。
 */
DECLARE_HOOK(android_vh_do_futex,
	TP_PROTO(int cmd,
		 unsigned int *flags,
		 u32 __user *uaddr2),
	TP_ARGS(cmd, flags, uaddr2));
DECLARE_HOOK(android_vh_futex_wait_start,
	TP_PROTO(unsigned int flags,
		 u32 bitset),
	TP_ARGS(flags, bitset));
DECLARE_HOOK(android_vh_futex_wait_end,
	TP_PROTO(unsigned int flags,
		 u32 bitset),
	TP_ARGS(flags, bitset));
DECLARE_HOOK(android_vh_futex_wake_traverse_plist,
	TP_PROTO(struct plist_head *chain, int *target_nr,
		 union futex_key key, u32 bitset),
	TP_ARGS(chain, target_nr, key, bitset));
DECLARE_HOOK(android_vh_futex_wake_this,
	TP_PROTO(int ret, int nr_wake, int target_nr,
		 struct task_struct *p),
	TP_ARGS(ret, nr_wake, target_nr, p));
DECLARE_HOOK(android_vh_futex_wake_up_q_finish,
	TP_PROTO(int nr_wake, int target_nr),
	TP_ARGS(nr_wake, target_nr));
DECLARE_HOOK(android_vh_futex_wait_queue_start,
	TP_PROTO(u32 __user *uaddr,
		 unsigned int flags,
		 u32 bitset),
	TP_ARGS(uaddr, flags, bitset));
#endif /* _TRACE_HOOK_FUTEX_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
