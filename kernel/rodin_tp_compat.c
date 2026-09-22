// SPDX-License-Identifier: GPL-2.0
/*
 * rodin_tp_compat.c - tracepoint / vendor hook 探针的 6.6 模块 ABI 兼容层
 *
 * 外围闭源厂商模块按 6.6 头文件编译并注册探针，6.18 上游对部分
 * tracepoint / android_vh / android_rvh 的 TP_PROTO 做了参数增删、移位或类型
 * 替换，模块二进制里固化的 6.6 探针签名因此失效：
 *   - 仅类型替换（参数个数与位置不变）：KCFI 直接 CFI failure
 *     （第七轮真机：mkp 的 task_newtask 探针，6.18 把 clone_flags 改为 u64，
 *      0.393s kthreadd 建线程即崩）；
 *   - 参数增删/移位：即使没有 CFI，错位实参也会被模块按 6.6 语义解引用
 *     （例：android_vh_cgroup_set_task 6.18 把 task 从第 2 参挪到第 3 参）。
 *
 * 处理策略（全部集中在本文件，不改任何 trace 头文件）：
 *   模块（非 core_kernel_text）对下表 tp 的注册，在
 *   tracepoint_probe_register{,_prio,_may_exist} / android_rvh_probe_register
 *   入口被改道进 side-list，同时在真实 tp 上以 6.18 原型注册一个内核 thunk；
 *   thunk 按 6.6 原型与参数映射回调模块探针（KCFI 哈希与模块固化值一致）。
 *   内核自身的注册（tracefs event / perf / BPF raw tp / in-tree）一律不改道。
 *
 * 已知语义降级（6.18 已删除、无法重建的实参，见各 thunk 注释）：
 *   - android_rvh_replace_next_task_fair: se=NULL、repick=&false、simple=false；
 *   - mmap_lock_start_locking/released: memcg_path=NULL；
 *   - android_rvh_set_user_nice: allowed=&true（6.18 已无否决语义）。
 */
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/cpumask.h>
#include <ufs/ufshcd.h>
#include "../drivers/ufs/core/ufs_trace_types.h"

bool rodin_tp66_reroute(struct tracepoint *tp, void *probe, void *data);
bool rodin_tp66_unreroute(struct tracepoint *tp, void *probe, void *data);

struct rq;
struct sched_entity;
struct cgroup;
struct binder_transaction;
struct scsi_device;

extern struct tracepoint __tracepoint_android_rvh_after_dequeue_task;
extern struct tracepoint __tracepoint_android_rvh_find_lowest_rq;
extern struct tracepoint __tracepoint_android_rvh_replace_next_task_fair;
extern struct tracepoint __tracepoint_android_rvh_set_user_nice;
extern struct tracepoint __tracepoint_android_vh_binder_restore_priority;
extern struct tracepoint __tracepoint_android_vh_cgroup_set_task;
extern struct tracepoint __tracepoint_android_vh_do_futex;
extern struct tracepoint __tracepoint_android_vh_mmput;
extern struct tracepoint __tracepoint_task_newtask;
extern struct tracepoint __tracepoint_mmap_lock_start_locking;
extern struct tracepoint __tracepoint_mmap_lock_released;
extern struct tracepoint __tracepoint_mmap_lock_acquire_returned;
extern struct tracepoint __tracepoint_sched_process_exit;
extern struct tracepoint __tracepoint_ufshcd_auto_bkops_state;
extern struct tracepoint __tracepoint_ufshcd_clk_gating;
extern struct tracepoint __tracepoint_ufshcd_clk_scaling;
extern struct tracepoint __tracepoint_ufshcd_command;
extern struct tracepoint __tracepoint_ufshcd_exception_event;
extern struct tracepoint __tracepoint_ufshcd_uic_command;
extern struct tracepoint __tracepoint_ufshcd_upiu;

struct rodin_tp66_probe {
	void *probe;
	void *data;
};

struct rodin_tp66_hook {
	struct tracepoint *tp;
	void *thunk;			/* 6.18 原型的内核回调 */
	bool installed;
	int nr;				/* 已用槽数（含空洞） */
	struct rodin_tp66_probe slot[4];
};

static DEFINE_MUTEX(rodin_tp66_lock);

/* ---- 各 hook 的 side-list 与 thunk ---- */

/* android_rvh_after_dequeue_task: 6.18 尾部新增 bool *dequeue_task_result */
static struct rodin_tp66_hook rodin_tp66_h_after_dequeue;
static void rodin_tp66_after_dequeue_thunk(void *__data, struct rq *rq,
		struct task_struct *p, int flags, bool *dequeue_task_result)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_after_dequeue.slot;
	int n = READ_ONCE(rodin_tp66_h_after_dequeue.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct rq *, struct task_struct *, int) =
			(void (*)(void *, struct rq *, struct task_struct *, int))e[i].probe;

		if (f)
			f(e[i].data, rq, p, flags);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_after_dequeue = {
	.tp = &__tracepoint_android_rvh_after_dequeue_task,
	.thunk = (void *)rodin_tp66_after_dequeue_thunk,
};

/* android_rvh_find_lowest_rq: 6.18 拆分 sched_ctx/exec_ctx，6.6 的 p 对应 exec_ctx */
static struct rodin_tp66_hook rodin_tp66_h_find_lowest_rq;
static void rodin_tp66_find_lowest_rq_thunk(void *__data,
		struct task_struct *sched_ctx, struct task_struct *exec_ctx,
		struct cpumask *local_cpu_mask, int ret, int *lowest_cpu)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_find_lowest_rq.slot;
	int n = READ_ONCE(rodin_tp66_h_find_lowest_rq.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct task_struct *, struct cpumask *, int, int *) =
			(void (*)(void *, struct task_struct *, struct cpumask *, int, int *))e[i].probe;

		if (f)
			f(e[i].data, exec_ctx, local_cpu_mask, ret, lowest_cpu);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_find_lowest_rq = {
	.tp = &__tracepoint_android_rvh_find_lowest_rq,
	.thunk = (void *)rodin_tp66_find_lowest_rq_thunk,
};

/*
 * android_rvh_replace_next_task_fair: 6.18 pick_next_task 重构后删除
 * se/repick/simple。se 传 NULL、repick 传局部 false（模块可写，内核侧忽略）、
 * simple 传 false。
 */
static struct rodin_tp66_hook rodin_tp66_h_replace_next;
static void rodin_tp66_replace_next_thunk(void *__data, struct rq *rq,
		struct task_struct **p, struct task_struct *prev)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_replace_next.slot;
	int n = READ_ONCE(rodin_tp66_h_replace_next.nr);
	struct sched_entity *se = NULL;
	bool repick = false;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct rq *, struct task_struct **,
			  struct sched_entity **, bool *, bool, struct task_struct *) =
			(void (*)(void *, struct rq *, struct task_struct **,
				  struct sched_entity **, bool *, bool,
				  struct task_struct *))e[i].probe;

		if (f)
			f(e[i].data, rq, p, &se, &repick, false, prev);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_replace_next = {
	.tp = &__tracepoint_android_rvh_replace_next_task_fair,
	.thunk = (void *)rodin_tp66_replace_next_thunk,
};

/* android_rvh_set_user_nice: 6.18 删除 bool *allowed；指向局部 true */
static struct rodin_tp66_hook rodin_tp66_h_set_user_nice;
static void rodin_tp66_set_user_nice_thunk(void *__data,
		struct task_struct *p, long *nice)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_set_user_nice.slot;
	int n = READ_ONCE(rodin_tp66_h_set_user_nice.nr);
	bool allowed = true;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct task_struct *, long *, bool *) =
			(void (*)(void *, struct task_struct *, long *, bool *))e[i].probe;

		if (f)
			f(e[i].data, p, nice, &allowed);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_set_user_nice = {
	.tp = &__tracepoint_android_rvh_set_user_nice,
	.thunk = (void *)rodin_tp66_set_user_nice_thunk,
};

/* android_vh_binder_restore_priority: 6.18 删除 task；6.6 调用点传 current */
static struct rodin_tp66_hook rodin_tp66_h_binder_restore;
static void rodin_tp66_binder_restore_thunk(void *__data,
		struct binder_transaction *t)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_binder_restore.slot;
	int n = READ_ONCE(rodin_tp66_h_binder_restore.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct binder_transaction *, struct task_struct *) =
			(void (*)(void *, struct binder_transaction *, struct task_struct *))e[i].probe;

		if (f)
			f(e[i].data, t, current);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_binder_restore = {
	.tp = &__tracepoint_android_vh_binder_restore_priority,
	.thunk = (void *)rodin_tp66_binder_restore_thunk,
};

/* android_vh_cgroup_set_task: 6.18 插入 cgrp 并把 task 挪到第 3 参 */
static struct rodin_tp66_hook rodin_tp66_h_cgroup_set_task;
static void rodin_tp66_cgroup_set_task_thunk(void *__data, int ret,
		struct cgroup *cgrp, struct task_struct *task, bool threadgroup)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_cgroup_set_task.slot;
	int n = READ_ONCE(rodin_tp66_h_cgroup_set_task.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, int, struct task_struct *) =
			(void (*)(void *, int, struct task_struct *))e[i].probe;

		if (f)
			f(e[i].data, ret, task);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_cgroup_set_task = {
	.tp = &__tracepoint_android_vh_cgroup_set_task,
	.thunk = (void *)rodin_tp66_cgroup_set_task_thunk,
};

/* android_vh_do_futex: 6.18 首部插入 uaddr */
static struct rodin_tp66_hook rodin_tp66_h_do_futex;
static void rodin_tp66_do_futex_thunk(void *__data, u32 __user *uaddr, int cmd,
		unsigned int *flags, u32 __user *uaddr2)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_do_futex.slot;
	int n = READ_ONCE(rodin_tp66_h_do_futex.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, int, unsigned int *, u32 __user *) =
			(void (*)(void *, int, unsigned int *, u32 __user *))e[i].probe;

		if (f)
			f(e[i].data, cmd, flags, uaddr2);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_do_futex = {
	.tp = &__tracepoint_android_vh_do_futex,
	.thunk = (void *)rodin_tp66_do_futex_thunk,
};

/* android_vh_mmput: 6.6 为 void *unused，6.18 改为 struct mm_struct *mm */
static struct rodin_tp66_hook rodin_tp66_h_mmput;
static void rodin_tp66_mmput_thunk(void *__data, struct mm_struct *mm)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_mmput.slot;
	int n = READ_ONCE(rodin_tp66_h_mmput.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, void *) =
			(void (*)(void *, void *))e[i].probe;

		if (f)
			f(e[i].data, mm);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_mmput = {
	.tp = &__tracepoint_android_vh_mmput,
	.thunk = (void *)rodin_tp66_mmput_thunk,
};

/* task_newtask: 6.18 clone_flags 改 u64（mkp/pidmap/cpuqos_v3/scheduler 探针） */
static struct rodin_tp66_hook rodin_tp66_h_task_newtask;
static void rodin_tp66_task_newtask_thunk(void *__data,
		struct task_struct *task, u64 clone_flags)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_task_newtask.slot;
	int n = READ_ONCE(rodin_tp66_h_task_newtask.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct task_struct *, unsigned long) =
			(void (*)(void *, struct task_struct *, unsigned long))e[i].probe;

		if (f)
			f(e[i].data, task, (unsigned long)clone_flags);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_task_newtask = {
	.tp = &__tracepoint_task_newtask,
	.thunk = (void *)rodin_tp66_task_newtask_thunk,
};

/* mmap_lock_start_locking/released: 6.18 删除 memcg_path，传 NULL（6.6 允许） */
static struct rodin_tp66_hook rodin_tp66_h_mmap_lock_start;
static void rodin_tp66_mmap_lock_start_thunk(void *__data,
		struct mm_struct *mm, bool write)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_mmap_lock_start.slot;
	int n = READ_ONCE(rodin_tp66_h_mmap_lock_start.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct mm_struct *, const char *, bool) =
			(void (*)(void *, struct mm_struct *, const char *, bool))e[i].probe;

		if (f)
			f(e[i].data, mm, NULL, write);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_mmap_lock_start = {
	.tp = &__tracepoint_mmap_lock_start_locking,
	.thunk = (void *)rodin_tp66_mmap_lock_start_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_mmap_lock_released;
static void rodin_tp66_mmap_lock_released_thunk(void *__data,
		struct mm_struct *mm, bool write)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_mmap_lock_released.slot;
	int n = READ_ONCE(rodin_tp66_h_mmap_lock_released.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct mm_struct *, const char *, bool) =
			(void (*)(void *, struct mm_struct *, const char *, bool))e[i].probe;

		if (f)
			f(e[i].data, mm, NULL, write);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_mmap_lock_released = {
	.tp = &__tracepoint_mmap_lock_released,
	.thunk = (void *)rodin_tp66_mmap_lock_released_thunk,
};

/* mmap_lock_acquire_returned: 6.18 删除 memcg_path，传 NULL（6.6 允许） */
static struct rodin_tp66_hook rodin_tp66_h_mmap_lock_acquire;
static void rodin_tp66_mmap_lock_acquire_thunk(void *__data,
		struct mm_struct *mm, bool write, bool success)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_mmap_lock_acquire.slot;
	int n = READ_ONCE(rodin_tp66_h_mmap_lock_acquire.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct mm_struct *, const char *, bool, bool) =
			(void (*)(void *, struct mm_struct *, const char *, bool, bool))e[i].probe;

		if (f)
			f(e[i].data, mm, NULL, write, success);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_mmap_lock_acquire = {
	.tp = &__tracepoint_mmap_lock_acquire_returned,
	.thunk = (void *)rodin_tp66_mmap_lock_acquire_thunk,
};

/* sched_process_exit: 6.18 尾部新增 bool group_dead */
static struct rodin_tp66_hook rodin_tp66_h_process_exit;
static void rodin_tp66_process_exit_thunk(void *__data,
		struct task_struct *p, bool group_dead)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_process_exit.slot;
	int n = READ_ONCE(rodin_tp66_h_process_exit.nr);
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, struct task_struct *) =
			(void (*)(void *, struct task_struct *))e[i].probe;

		if (f)
			f(e[i].data, p);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_process_exit = {
	.tp = &__tracepoint_sched_process_exit,
	.thunk = (void *)rodin_tp66_process_exit_thunk,
};

/*
 * ufshcd_*: 6.18 上游把首参 const char *dev_name 换成 struct ufs_hba *hba
 * （ufshcd_command 还在首部插入 sdev）。dev_name 由 hba 现算，与 6.6 调用点一致。
 */
static struct rodin_tp66_hook rodin_tp66_h_ufs_abo;
static void rodin_tp66_ufs_abo_thunk(void *__data, struct ufs_hba *hba,
		const char *state)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_abo.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_abo.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, const char *) =
			(void (*)(void *, const char *, const char *))e[i].probe;

		if (f)
			f(e[i].data, dn, state);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_abo = {
	.tp = &__tracepoint_ufshcd_auto_bkops_state,
	.thunk = (void *)rodin_tp66_ufs_abo_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_ufs_gating;
static void rodin_tp66_ufs_gating_thunk(void *__data, struct ufs_hba *hba,
		int state)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_gating.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_gating.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, int) =
			(void (*)(void *, const char *, int))e[i].probe;

		if (f)
			f(e[i].data, dn, state);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_gating = {
	.tp = &__tracepoint_ufshcd_clk_gating,
	.thunk = (void *)rodin_tp66_ufs_gating_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_ufs_scaling;
static void rodin_tp66_ufs_scaling_thunk(void *__data, struct ufs_hba *hba,
		const char *state, const char *clk, u32 prev_state, u32 curr_state)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_scaling.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_scaling.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, const char *, const char *, u32, u32) =
			(void (*)(void *, const char *, const char *, const char *, u32, u32))e[i].probe;

		if (f)
			f(e[i].data, dn, state, clk, prev_state, curr_state);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_scaling = {
	.tp = &__tracepoint_ufshcd_clk_scaling,
	.thunk = (void *)rodin_tp66_ufs_scaling_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_ufs_command;
static void rodin_tp66_ufs_command_thunk(void *__data,
		struct scsi_device *sdev, struct ufs_hba *hba,
		enum ufs_trace_str_t str_t, unsigned int tag, u32 doorbell,
		u32 hwq_id, int transfer_len, u32 intr, u64 lba, u8 opcode,
		u8 group_id)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_command.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_command.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, enum ufs_trace_str_t, unsigned int,
			  u32, u32, int, u32, u64, u8, u8) =
			(void (*)(void *, const char *, enum ufs_trace_str_t,
				  unsigned int, u32, u32, int, u32, u64, u8,
				  u8))e[i].probe;

		if (f)
			f(e[i].data, dn, str_t, tag, doorbell, hwq_id,
			  transfer_len, intr, lba, opcode, group_id);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_command = {
	.tp = &__tracepoint_ufshcd_command,
	.thunk = (void *)rodin_tp66_ufs_command_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_ufs_exception;
static void rodin_tp66_ufs_exception_thunk(void *__data, struct ufs_hba *hba,
		u16 status)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_exception.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_exception.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, u16) =
			(void (*)(void *, const char *, u16))e[i].probe;

		if (f)
			f(e[i].data, dn, status);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_exception = {
	.tp = &__tracepoint_ufshcd_exception_event,
	.thunk = (void *)rodin_tp66_ufs_exception_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_ufs_uic;
static void rodin_tp66_ufs_uic_thunk(void *__data, struct ufs_hba *hba,
		enum ufs_trace_str_t str_t, u32 cmd, u32 arg1, u32 arg2, u32 arg3)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_uic.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_uic.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, enum ufs_trace_str_t, u32, u32, u32, u32) =
			(void (*)(void *, const char *, enum ufs_trace_str_t, u32, u32,
				  u32, u32))e[i].probe;

		if (f)
			f(e[i].data, dn, str_t, cmd, arg1, arg2, arg3);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_uic = {
	.tp = &__tracepoint_ufshcd_uic_command,
	.thunk = (void *)rodin_tp66_ufs_uic_thunk,
};

static struct rodin_tp66_hook rodin_tp66_h_ufs_upiu;
static void rodin_tp66_ufs_upiu_thunk(void *__data, struct ufs_hba *hba,
		enum ufs_trace_str_t str_t, void *hdr, void *tsf,
		enum ufs_trace_tsf_t tsf_t)
{
	struct rodin_tp66_probe *e = rodin_tp66_h_ufs_upiu.slot;
	int n = READ_ONCE(rodin_tp66_h_ufs_upiu.nr);
	const char *dn = hba ? dev_name(hba->dev) : NULL;
	int i;

	for (i = 0; i < n; i++) {
		void (*f)(void *, const char *, enum ufs_trace_str_t, void *, void *,
			  enum ufs_trace_tsf_t) =
			(void (*)(void *, const char *, enum ufs_trace_str_t, void *,
				  void *, enum ufs_trace_tsf_t))e[i].probe;

		if (f)
			f(e[i].data, dn, str_t, hdr, tsf, tsf_t);
	}
}
static struct rodin_tp66_hook rodin_tp66_h_ufs_upiu = {
	.tp = &__tracepoint_ufshcd_upiu,
	.thunk = (void *)rodin_tp66_ufs_upiu_thunk,
};

/* ---- 表与入口 ---- */

static struct rodin_tp66_hook *rodin_tp66_table[] = {
	&rodin_tp66_h_after_dequeue,
	&rodin_tp66_h_find_lowest_rq,
	&rodin_tp66_h_replace_next,
	&rodin_tp66_h_set_user_nice,
	&rodin_tp66_h_binder_restore,
	&rodin_tp66_h_cgroup_set_task,
	&rodin_tp66_h_do_futex,
	&rodin_tp66_h_mmput,
	&rodin_tp66_h_task_newtask,
	&rodin_tp66_h_mmap_lock_start,
	&rodin_tp66_h_mmap_lock_released,
	&rodin_tp66_h_mmap_lock_acquire,
	&rodin_tp66_h_process_exit,
	&rodin_tp66_h_ufs_abo,
	&rodin_tp66_h_ufs_gating,
	&rodin_tp66_h_ufs_scaling,
	&rodin_tp66_h_ufs_command,
	&rodin_tp66_h_ufs_exception,
	&rodin_tp66_h_ufs_uic,
	&rodin_tp66_h_ufs_upiu,
};

static struct rodin_tp66_hook *rodin_tp66_find(struct tracepoint *tp)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rodin_tp66_table); i++)
		if (rodin_tp66_table[i]->tp == tp)
			return rodin_tp66_table[i];
	return NULL;
}

/*
 * 模块探针注册改道入口（由 kernel/tracepoint.c 的公开注册函数调用，在拿
 * tracepoints_mutex 之前）。返回 true 表示注册已被本层消费。
 */
bool rodin_tp66_reroute(struct tracepoint *tp, void *probe, void *data)
{
	struct rodin_tp66_hook *h;
	int i, ret;

	h = rodin_tp66_find(tp);
	/* 内核自身的注册（thunk/event/perf/bpf）不改道，须在拿锁前判断 */
	if (!h || core_kernel_text((unsigned long)probe))
		return false;

	mutex_lock(&rodin_tp66_lock);
	for (i = 0; i < h->nr; i++)
		if (!h->slot[i].probe)
			break;
	if (i == h->nr) {
		if (h->nr >= ARRAY_SIZE(h->slot)) {
			pr_warn("rodin_tp66: %s: side-list 满，探针注册被忽略\n",
				h->tp->name);
			mutex_unlock(&rodin_tp66_lock);
			/* 仍消费注册：直连原型不符的探针在派发时必然 CFI 崩溃 */
			return true;
		}
		h->nr++;
	}

	ret = 0;
	if (!h->installed)
		ret = tracepoint_probe_register_prio(h->tp, h->thunk, NULL,
						     TRACEPOINT_DEFAULT_PRIO);
	if (ret) {
		pr_warn("rodin_tp66: %s: thunk 注册失败 %d\n", h->tp->name, ret);
		mutex_unlock(&rodin_tp66_lock);
		return true;
	}
	h->installed = true;
	h->slot[i].probe = probe;
	h->slot[i].data = data;
	smp_wmb();
	mutex_unlock(&rodin_tp66_lock);
	return true;
}

/*
 * 模块探针注销改道入口（由 tracepoint_probe_unregister 调用）。返回 true 表示
 * 该探针在本层，调用方直接返回即可。
 */
bool rodin_tp66_unreroute(struct tracepoint *tp, void *probe, void *data)
{
	struct rodin_tp66_hook *h;
	int i;

	h = rodin_tp66_find(tp);
	if (!h)
		return false;

	mutex_lock(&rodin_tp66_lock);
	for (i = 0; i < h->nr; i++) {
		if (h->slot[i].probe == probe) {
			WRITE_ONCE(h->slot[i].probe, NULL);
			h->slot[i].data = NULL;
			smp_wmb();
			mutex_unlock(&rodin_tp66_lock);
			return true;
		}
	}
	mutex_unlock(&rodin_tp66_lock);
	return false;
}
