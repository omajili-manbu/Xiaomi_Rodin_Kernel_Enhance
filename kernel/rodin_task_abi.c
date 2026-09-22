// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6 ABI 冻结：struct task_struct 布局的编译期校验。
 *
 * 背景：厂商预编译模块（596 个 .ko）把 6.6 的字段偏移编译进代码里。6.18 重排了
 * task_struct（pid 0x618 -> 0x758、cred 0x820 -> 0x950、mm 0x5a0 -> 0x6d8 …），
 * 真机上 mkp.ko 因此把 current->pid 读成垃圾值并触发防御性 panic（panic 现场见
 * console-ramoops-0：probe_android_vh_check_file_open -> check_cred ->
 * handle_mkp_err_action，"pid is overflow"）。
 *
 * 处理方式（与 struct module / struct device 的既有做法一致）：
 *   - 6.18 新增的成员统一后置到结构体末尾；
 *   - 6.6 有、6.18 删掉的成员用同尺寸占位补回；
 *   - 尺寸与 6.6 不同的内嵌成员（sched_dl_entity/sched_info/task_io_accounting/
 *     tlbflush_unmap_batch）整体后置，原地留 6.6 尺寸占位。
 * 下面把所有 6.6 成员的偏移钉死：上游合并/配置改动一旦把偏移挪走，直接编译失败，
 * 而不是等到真机上某个厂商模块静默读到野值。
 *
 * 字段偏移来源：6.6 树（kernel-rodin-merge）vmlinux 的 BTF。
 */
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/mm_types.h>
#include <linux/sched/task.h>

#define RODIN_TS_OFF(member, off)					\
	static_assert(offsetof(struct task_struct, member) == (off),	\
		      "rodin 6.6 ABI 破坏: struct task_struct::" #member)

RODIN_TS_OFF(thread_info, 0x000);
RODIN_TS_OFF(__state, 0x030);
RODIN_TS_OFF(saved_state, 0x034);
RODIN_TS_OFF(stack, 0x038);
RODIN_TS_OFF(usage, 0x040);
RODIN_TS_OFF(flags, 0x044);
RODIN_TS_OFF(ptrace, 0x048);
RODIN_TS_OFF(on_cpu, 0x04c);
RODIN_TS_OFF(wake_entry, 0x050);
RODIN_TS_OFF(wakee_flips, 0x060);
RODIN_TS_OFF(wakee_flip_decay_ts, 0x068);
RODIN_TS_OFF(last_wakee, 0x070);
RODIN_TS_OFF(recent_used_cpu, 0x078);
RODIN_TS_OFF(wake_cpu, 0x07c);
RODIN_TS_OFF(on_rq, 0x080);
RODIN_TS_OFF(prio, 0x084);
RODIN_TS_OFF(static_prio, 0x088);
RODIN_TS_OFF(normal_prio, 0x08c);
RODIN_TS_OFF(rt_priority, 0x090);
RODIN_TS_OFF(se, 0x0c0);
RODIN_TS_OFF(rt, 0x200);
RODIN_TS_OFF(__rodin_66_slot_dl, 0x250);	/* 6.6: dl —— 6.18 的 sched_dl_entity 长到 288B，真身后置、原地 240B 占位 */
RODIN_TS_OFF(sched_class, 0x340);
RODIN_TS_OFF(sched_task_group, 0x348);
RODIN_TS_OFF(uclamp_req, 0x350);
RODIN_TS_OFF(uclamp, 0x358);
RODIN_TS_OFF(stats, 0x380);
RODIN_TS_OFF(preempt_notifiers, 0x480);
RODIN_TS_OFF(policy, 0x488);
RODIN_TS_OFF(nr_cpus_allowed, 0x48c);
RODIN_TS_OFF(cpus_ptr, 0x490);
RODIN_TS_OFF(user_cpus_ptr, 0x498);
RODIN_TS_OFF(cpus_mask, 0x4a0);
RODIN_TS_OFF(migration_pending, 0x4a8);
RODIN_TS_OFF(migration_disabled, 0x4b0);
RODIN_TS_OFF(migration_flags, 0x4b2);
RODIN_TS_OFF(rcu_read_lock_nesting, 0x4b4);
RODIN_TS_OFF(rcu_read_unlock_special, 0x4b8);
RODIN_TS_OFF(rcu_node_entry, 0x4c0);
RODIN_TS_OFF(rcu_blocked_node, 0x4d0);
RODIN_TS_OFF(rcu_tasks_nvcsw, 0x4d8);
RODIN_TS_OFF(rcu_tasks_holdout, 0x4e0);
RODIN_TS_OFF(rcu_tasks_idx, 0x4e1);
RODIN_TS_OFF(rcu_tasks_idle_cpu, 0x4e4);
RODIN_TS_OFF(rcu_tasks_holdout_list, 0x4e8);
RODIN_TS_OFF(trc_reader_nesting, 0x4f8);
RODIN_TS_OFF(trc_ipi_to_cpu, 0x4fc);
RODIN_TS_OFF(trc_reader_special, 0x500);
RODIN_TS_OFF(trc_holdout_list, 0x508);
RODIN_TS_OFF(trc_blkd_node, 0x518);
RODIN_TS_OFF(trc_blkd_cpu, 0x528);
RODIN_TS_OFF(__rodin_66_slot_sched_info, 0x530);	/* 6.6: sched_info —— 6.18 的 sched_info 长到 48B，真身后置、原地 32B 占位 */
RODIN_TS_OFF(tasks, 0x550);
RODIN_TS_OFF(pushable_tasks, 0x560);
RODIN_TS_OFF(pushable_dl_tasks, 0x588);
RODIN_TS_OFF(mm, 0x5a0);
RODIN_TS_OFF(active_mm, 0x5a8);
RODIN_TS_OFF(exit_state, 0x5b0);
RODIN_TS_OFF(exit_code, 0x5b4);
RODIN_TS_OFF(exit_signal, 0x5b8);
RODIN_TS_OFF(pdeath_signal, 0x5bc);
RODIN_TS_OFF(jobctl, 0x5c0);
RODIN_TS_OFF(personality, 0x5c8);
RODIN_TS_OFF(atomic_flags, 0x5d8);
RODIN_TS_OFF(restart_block, 0x5e0);
RODIN_TS_OFF(pid, 0x618);
RODIN_TS_OFF(tgid, 0x61c);
RODIN_TS_OFF(stack_canary, 0x620);
RODIN_TS_OFF(real_parent, 0x628);
RODIN_TS_OFF(parent, 0x630);
RODIN_TS_OFF(children, 0x638);
RODIN_TS_OFF(sibling, 0x648);
RODIN_TS_OFF(group_leader, 0x658);
RODIN_TS_OFF(ptraced, 0x660);
RODIN_TS_OFF(ptrace_entry, 0x670);
RODIN_TS_OFF(thread_pid, 0x680);
RODIN_TS_OFF(pid_links, 0x688);
RODIN_TS_OFF(__rodin_66_slot_thread_group, 0x6c8);	/* 6.6: thread_group —— 6.18 删除了该成员，原地留 16B 占位 */
RODIN_TS_OFF(thread_node, 0x6d8);
RODIN_TS_OFF(vfork_done, 0x6e8);
RODIN_TS_OFF(set_child_tid, 0x6f0);
RODIN_TS_OFF(clear_child_tid, 0x6f8);
RODIN_TS_OFF(worker_private, 0x700);
RODIN_TS_OFF(utime, 0x708);
RODIN_TS_OFF(stime, 0x710);
RODIN_TS_OFF(gtime, 0x718);
RODIN_TS_OFF(time_in_state, 0x720);
RODIN_TS_OFF(max_state, 0x728);
RODIN_TS_OFF(prev_cputime, 0x730);
RODIN_TS_OFF(nvcsw, 0x748);
RODIN_TS_OFF(nivcsw, 0x750);
RODIN_TS_OFF(start_time, 0x758);
RODIN_TS_OFF(start_boottime, 0x760);
RODIN_TS_OFF(min_flt, 0x768);
RODIN_TS_OFF(maj_flt, 0x770);
RODIN_TS_OFF(posix_cputimers, 0x778);
RODIN_TS_OFF(posix_cputimers_work, 0x7c8);
RODIN_TS_OFF(ptracer_cred, 0x810);
RODIN_TS_OFF(real_cred, 0x818);
RODIN_TS_OFF(cred, 0x820);
RODIN_TS_OFF(cached_requested_key, 0x828);
RODIN_TS_OFF(comm, 0x830);
RODIN_TS_OFF(nameidata, 0x840);
RODIN_TS_OFF(last_switch_count, 0x848);
RODIN_TS_OFF(last_switch_time, 0x850);
RODIN_TS_OFF(fs, 0x858);
RODIN_TS_OFF(files, 0x860);
RODIN_TS_OFF(io_uring, 0x868);
RODIN_TS_OFF(nsproxy, 0x870);
RODIN_TS_OFF(signal, 0x878);
RODIN_TS_OFF(sighand, 0x880);
RODIN_TS_OFF(blocked, 0x888);
RODIN_TS_OFF(real_blocked, 0x890);
RODIN_TS_OFF(saved_sigmask, 0x898);
RODIN_TS_OFF(pending, 0x8a0);
RODIN_TS_OFF(sas_ss_sp, 0x8b8);
RODIN_TS_OFF(sas_ss_size, 0x8c0);
RODIN_TS_OFF(sas_ss_flags, 0x8c8);
RODIN_TS_OFF(task_works, 0x8d0);
RODIN_TS_OFF(audit_context, 0x8d8);
RODIN_TS_OFF(loginuid, 0x8e0);
RODIN_TS_OFF(sessionid, 0x8e4);
RODIN_TS_OFF(seccomp, 0x8e8);
RODIN_TS_OFF(syscall_dispatch, 0x8f8);
RODIN_TS_OFF(parent_exec_id, 0x8f8);
RODIN_TS_OFF(self_exec_id, 0x900);
RODIN_TS_OFF(alloc_lock, 0x908);
RODIN_TS_OFF(pi_lock, 0x90c);
RODIN_TS_OFF(wake_q, 0x910);
RODIN_TS_OFF(wake_q_count, 0x918);
RODIN_TS_OFF(pi_waiters, 0x920);
RODIN_TS_OFF(pi_top_task, 0x930);
RODIN_TS_OFF(pi_blocked_on, 0x938);
RODIN_TS_OFF(journal_info, 0x940);
RODIN_TS_OFF(bio_list, 0x948);
RODIN_TS_OFF(plug, 0x950);
RODIN_TS_OFF(reclaim_state, 0x958);
RODIN_TS_OFF(io_context, 0x960);
RODIN_TS_OFF(capture_control, 0x968);
RODIN_TS_OFF(ptrace_message, 0x970);
RODIN_TS_OFF(last_siginfo, 0x978);
RODIN_TS_OFF(__rodin_66_slot_ioac, 0x980);	/* 6.6: ioac —— 6.18 的 task_io_accounting 缩到 56B，真身后置、原地 64B 占位 */
RODIN_TS_OFF(psi_flags, 0x9c0);
RODIN_TS_OFF(acct_rss_mem1, 0x9c8);
RODIN_TS_OFF(acct_vm_mem1, 0x9d0);
RODIN_TS_OFF(acct_timexpd, 0x9d8);
RODIN_TS_OFF(mems_allowed, 0x9e0);
RODIN_TS_OFF(mems_allowed_seq, 0x9e8);
RODIN_TS_OFF(cpuset_mem_spread_rotor, 0x9ec);
RODIN_TS_OFF(__rodin_66_slot_cpuset_slab_spread_rotor, 0x9f0);	/* 6.6: cpuset_slab_spread_rotor —— 6.18 删除了该成员，原地留 4B 占位 */
RODIN_TS_OFF(cgroups, 0x9f8);
RODIN_TS_OFF(cg_list, 0xa00);
RODIN_TS_OFF(robust_list, 0xa10);
RODIN_TS_OFF(compat_robust_list, 0xa18);
RODIN_TS_OFF(pi_state_list, 0xa20);
RODIN_TS_OFF(pi_state_cache, 0xa30);
RODIN_TS_OFF(futex_exit_mutex, 0xa38);
RODIN_TS_OFF(futex_state, 0xa68);
RODIN_TS_OFF(perf_event_ctxp, 0xa70);
RODIN_TS_OFF(perf_event_mutex, 0xa78);
RODIN_TS_OFF(perf_event_list, 0xaa8);
RODIN_TS_OFF(__rodin_66_slot_tlb_ubc, 0xab8);	/* 6.6: tlb_ubc —— 6.18 的 tlbflush_unmap_batch 长到 24B，真身后置、原地 2B 占位 */
RODIN_TS_OFF(splice_pipe, 0xac0);
RODIN_TS_OFF(task_frag, 0xac8);
RODIN_TS_OFF(delays, 0xad8);
RODIN_TS_OFF(nr_dirtied, 0xae0);
RODIN_TS_OFF(nr_dirtied_pause, 0xae4);
RODIN_TS_OFF(dirty_paused_when, 0xae8);
RODIN_TS_OFF(timer_slack_ns, 0xaf0);
RODIN_TS_OFF(default_timer_slack_ns, 0xaf8);
RODIN_TS_OFF(kunit_test, 0xb00);
RODIN_TS_OFF(trace_recursion, 0xb08);
RODIN_TS_OFF(__rodin_66_slot_memcg_in_oom, 0xb10);	/* 6.6: memcg_in_oom —— 6.18 移到 CONFIG_MEMCG_V1 下（未开），原地留 8B 占位 */
RODIN_TS_OFF(__rodin_66_slot_memcg_oom_gfp_mask, 0xb18);	/* 6.6: memcg_oom_gfp_mask —— 6.18 删除，原地留 4B 占位 */
RODIN_TS_OFF(__rodin_66_slot_memcg_oom_order, 0xb1c);	/* 6.6: memcg_oom_order —— 6.18 删除，原地留 4B 占位 */
RODIN_TS_OFF(memcg_nr_pages_over_high, 0xb20);
RODIN_TS_OFF(active_memcg, 0xb28);
RODIN_TS_OFF(throttle_disk, 0xb30);
RODIN_TS_OFF(utask, 0xb38);
RODIN_TS_OFF(kmap_ctrl, 0xb40);
RODIN_TS_OFF(rcu, 0xb40);
RODIN_TS_OFF(rcu_users, 0xb50);
RODIN_TS_OFF(pagefault_disabled, 0xb54);
RODIN_TS_OFF(oom_reaper_list, 0xb58);
RODIN_TS_OFF(oom_reaper_timer, 0xb60);
RODIN_TS_OFF(stack_vm_area, 0xb98);
RODIN_TS_OFF(stack_refcount, 0xba0);
RODIN_TS_OFF(security, 0xba8);
RODIN_TS_OFF(bpf_storage, 0xbb0);
RODIN_TS_OFF(bpf_ctx, 0xbb8);
RODIN_TS_OFF(android_vendor_data1, 0xbc0);
RODIN_TS_OFF(android_oem_data1, 0xdc0);
RODIN_TS_OFF(kretprobe_instances, 0xdf0);
RODIN_TS_OFF(dmabuf_info, 0xdf8);
RODIN_TS_OFF(__kabi_reserved1, 0xdf8);	/* 6.6: android_kabi_reserved1 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved2, 0xe00);	/* 6.6: android_kabi_reserved2 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved3, 0xe08);	/* 6.6: android_kabi_reserved3 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved4, 0xe10);	/* 6.6: android_kabi_reserved4 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved5, 0xe18);	/* 6.6: android_kabi_reserved5 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved6, 0xe20);	/* 6.6: android_kabi_reserved6 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved7, 0xe28);	/* 6.6: android_kabi_reserved7 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(__kabi_reserved8, 0xe30);	/* 6.6: android_kabi_reserved8 —— 6.18 的宏生成的字段名改成了 __kabi_reservedN */
RODIN_TS_OFF(thread, 0xe40);

/*
 * 冻结边界：6.6 的最后一个成员是 thread（1120B）。
 * 6.18 新增的成员必须全部落在这个边界之后。
 */
#define RODIN_TASK_6_6_FROZEN_END	0x12c0

static_assert(offsetof(struct task_struct, thread) == 0xe40,
	      "rodin 6.6 ABI 破坏: task_struct::thread");
static_assert(sizeof(struct task_struct) >= RODIN_TASK_6_6_FROZEN_END,
	      "rodin 6.6 ABI 破坏: task_struct 尾部被裁短");

/* 6.18 新增成员（后置区）必须全部在冻结边界之后 */
#define RODIN_TS_TAIL_OFF(member)					\
	static_assert(offsetof(struct task_struct, member) >=		\
		      RODIN_TASK_6_6_FROZEN_END,			\
		      "rodin 6.6 ABI 破坏: 6.18 新成员 " #member " 跑进了 6.6 前缀")

RODIN_TS_TAIL_OFF(alloc_tag);
RODIN_TS_TAIL_OFF(max_allowed_capacity);
RODIN_TS_TAIL_OFF(blocker);
RODIN_TS_TAIL_OFF(faults_disabled_mapping);
RODIN_TS_TAIL_OFF(is_blocked);
RODIN_TS_TAIL_OFF(dl_server);
RODIN_TS_TAIL_OFF(blocked_donor);
RODIN_TS_TAIL_OFF(blocked_lock);
RODIN_TS_TAIL_OFF(objcg);
RODIN_TS_TAIL_OFF(bpf_net_context);
RODIN_TS_TAIL_OFF(perf_ctx_data);
RODIN_TS_TAIL_OFF(rcu_tasks_exit_cpu);
RODIN_TS_TAIL_OFF(perf_recursion);
RODIN_TS_TAIL_OFF(blocked_on);
RODIN_TS_TAIL_OFF(sched_throttle_work);
RODIN_TS_TAIL_OFF(throttle_node);
RODIN_TS_TAIL_OFF(rcu_tasks_exit_list);
RODIN_TS_TAIL_OFF(migration_node);
RODIN_TS_TAIL_OFF(blocked_head);
RODIN_TS_TAIL_OFF(blocked_node);
RODIN_TS_TAIL_OFF(blocked_activation_node);
RODIN_TS_TAIL_OFF(sleeping_owner);
RODIN_TS_TAIL_OFF(throttled);
#ifdef CONFIG_SCHED_CLASS_EXT
RODIN_TS_TAIL_OFF(scx);
#endif
/* 后置的真身 */
RODIN_TS_TAIL_OFF(dl);
RODIN_TS_TAIL_OFF(sched_info);
RODIN_TS_TAIL_OFF(ioac);
RODIN_TS_TAIL_OFF(tlb_ubc);
