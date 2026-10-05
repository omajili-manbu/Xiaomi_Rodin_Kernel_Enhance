// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6 ABI 冻结：mm 数据结构布局的编译期校验。
 *
 * 背景：厂商预编译模块按 6.6 内核编译，直接解引用 pg_data_t / mem_cgroup /
 * mem_cgroup_per_node 的 GKI 厂商预留字段。6.18 上游整体缩小/移动了这些
 * 结构体，按 6.6 偏移的访问会越过结构体末尾读到相邻全局变量的内容——真机
 * 表现为 zram.ko xswapd 初始化：读 pgdat+0x21f0（6.6 的 android_oem_data1）
 * 得到越界野指针，init_waitqueue_head(&xdat->wait) 对只读内存写入而 panic
 * （pc=__init_waitqueue_head, lr=run_xswapd，console-ramoops-0 第六轮）。
 *
 * 修复（include/linux/mmzone.h / memcontrol.h 的 6.6 兼容尾区）：
 *   - pg_data_t:     0x2100 node_id 镜像（rodin_node_id66）、
 *                    0x21f0 android_oem_data1、0x2260 6.6 形状 __lruvec66；
 *   - mem_cgroup:    0x8f0 android_oem_data1[2]、0x900 nodeinfo[]；
 *   - mem_cgroup_per_node: 0x638 lruvec pgdat 回指针
 *                    （rodin_lruvec_pgdat），0x648/0x650 为 6.6
 *                    lruvec_stats.state[0..1] 的零填充降级位。
 *   - enum node_stat_item 恢复 6.6 序（NR_WRITEBACK_TEMP 占位），
 *     供 blocktag/mpbe 的编译期下标使用。
 * 偏移取自 6.6 树（kernel-rodin-merge）vmlinux 的 BTF：
 *   pglist_data.android_oem_data1 @ 0x21f0 / node_id @ 0x2100 /
 *   __lruvec @ 0x2260（lruvec.pgdat @ 0x638，size 0x640）；
 *   mem_cgroup.android_oem_data1[2] @ 0x8f0 / nodeinfo[] @ 0x900；
 *   mem_cgroup_per_node（lruvec@0，size 0x640 → state[] @ 0x648）。
 */
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/memcontrol.h>

#define RODIN_MM_OFF(type, member, off)					\
	static_assert(offsetof(type, member) == (off),			\
		      "rodin 6.6 ABI 破坏: " #type "::" #member)

/* pg_data_t：zram xswapd 的 6.6 偏移锚点 */
RODIN_MM_OFF(struct pglist_data, rodin_node_id66, 0x2100);
RODIN_MM_OFF(struct pglist_data, android_oem_data1, 0x21f0);
RODIN_MM_OFF(struct pglist_data, __lruvec66, 0x2260);

/* mem_cgroup：xswapd 用 android_oem_data1[1] 挂 per-memcg 控制块，
 * nodeinfo[nid] 取 per-node 对象。
 * MEMCG_V1=y 时 v1 字段组回归使尾区自然后移：blob zram/zsmalloc 已
 * 内建+skip 退役（177ddafc25cc，真机 "rodin: zram is built-in, skipping
 * load"），6.6 硬偏移无消费者，内核按字段名/struct_size 自洽，不锁偏移。 */
#ifndef CONFIG_MEMCG_V1
RODIN_MM_OFF(struct mem_cgroup, android_oem_data1, 0x8f0);
RODIN_MM_OFF(struct mem_cgroup, nodeinfo, 0x900);

/* mem_cgroup_per_node：xswapd 按 6.6 lruvec.pgdat 偏移同步回指针 */
RODIN_MM_OFF(struct mem_cgroup_per_node, rodin_lruvec_pgdat, 0x638);
#endif

/* node_stat_item：blocktag/mpbe 按 6.6 枚举序的编译期下标 */
static_assert(NR_FILE_PAGES == 19);
static_assert(NR_INACTIVE_ANON == 0 && NR_ACTIVE_ANON == 1 &&
	      NR_INACTIVE_FILE == 2);

/*
 * mm_struct：mitee optee_open_session 调用者鉴权链
 * current->active_mm(0x5a8) -> exe_file(0x418) -> file.f_path(0xa8) -> d_path。
 * 6.18 在 exe_file 之前新增 futex 组（104B）、saved_auxv 50->54 项（32B）、
 * mm_context_t 40->48B（8B），并删除 get_unmapped_area（8B），exe_file
 * 漂到 0x4a8：#188 真机 7.099s keymaster HAL d_path Oops（记账本 A-57）。
 * 处理照 task_struct 成法：mm_types.h 声明重排，6.6 冻结边界 0x4c0，
 * 真身（saved_auxv/context/vma_writer_wait/futex 组/iommu_mm/mm_id）后置。
 * 偏移取自 6.6 树（kernel-rodin-merge）vmlinux 的 BTF。
 */
#define RODIN_MMS_OFF(member, off)					\
	static_assert(offsetof(struct mm_struct, member) == (off),	\
		      "rodin 6.6 ABI 破坏: struct mm_struct::" #member)

RODIN_MMS_OFF(mm_mt, 0x040);
RODIN_MMS_OFF(__rodin_66_slot_get_unmapped_area, 0x050);
RODIN_MMS_OFF(mmap_base, 0x058);
RODIN_MMS_OFF(mmap_legacy_base, 0x060);
RODIN_MMS_OFF(task_size, 0x068);
RODIN_MMS_OFF(pgd, 0x070);
RODIN_MMS_OFF(membarrier_state, 0x078);
RODIN_MMS_OFF(mm_users, 0x07c);
RODIN_MMS_OFF(pgtables_bytes, 0x080);
RODIN_MMS_OFF(map_count, 0x088);
RODIN_MMS_OFF(page_table_lock, 0x08c);
RODIN_MMS_OFF(mmap_lock, 0x090);
RODIN_MMS_OFF(mmlist, 0x0d0);
RODIN_MMS_OFF(mm_lock_seq, 0x0e0);
RODIN_MMS_OFF(hiwater_rss, 0x0e8);
RODIN_MMS_OFF(hiwater_vm, 0x0f0);
RODIN_MMS_OFF(total_vm, 0x0f8);
RODIN_MMS_OFF(locked_vm, 0x100);
RODIN_MMS_OFF(pinned_vm, 0x108);
RODIN_MMS_OFF(data_vm, 0x110);
RODIN_MMS_OFF(exec_vm, 0x118);
RODIN_MMS_OFF(stack_vm, 0x120);
RODIN_MMS_OFF(def_flags, 0x128);
RODIN_MMS_OFF(write_protect_seq, 0x130);
RODIN_MMS_OFF(arg_lock, 0x134);
RODIN_MMS_OFF(start_code, 0x138);
RODIN_MMS_OFF(end_code, 0x140);
RODIN_MMS_OFF(start_data, 0x148);
RODIN_MMS_OFF(end_data, 0x150);
RODIN_MMS_OFF(start_brk, 0x158);
RODIN_MMS_OFF(brk, 0x160);
RODIN_MMS_OFF(start_stack, 0x168);
RODIN_MMS_OFF(arg_start, 0x170);
RODIN_MMS_OFF(arg_end, 0x178);
RODIN_MMS_OFF(env_start, 0x180);
RODIN_MMS_OFF(env_end, 0x188);
RODIN_MMS_OFF(__rodin_66_slot_saved_auxv, 0x190);
RODIN_MMS_OFF(rss_stat, 0x320);
RODIN_MMS_OFF(binfmt, 0x3c0);
RODIN_MMS_OFF(__rodin_66_slot_context, 0x3c8);
RODIN_MMS_OFF(flags, 0x3f0);
RODIN_MMS_OFF(ioctx_lock, 0x3f8);
RODIN_MMS_OFF(ioctx_table, 0x400);
RODIN_MMS_OFF(owner, 0x408);
RODIN_MMS_OFF(user_ns, 0x410);
RODIN_MMS_OFF(exe_file, 0x418);
RODIN_MMS_OFF(notifier_subscriptions, 0x420);
RODIN_MMS_OFF(tlb_flush_pending, 0x428);
RODIN_MMS_OFF(tlb_flush_batched, 0x42c);
RODIN_MMS_OFF(uprobes_state, 0x430);
RODIN_MMS_OFF(async_put_work, 0x438);
RODIN_MMS_OFF(lru_gen, 0x468);
RODIN_MMS_OFF(__kabi_reserved1, 0x488);
RODIN_MMS_OFF(dmabuf_info, 0x490);

/* 冻结边界：6.6 匿名结构在 0x4c0 结束；6.18 真身成员全部在其后 */
static_assert(offsetof(struct mm_struct, saved_auxv) == 0x4c0,
	      "rodin 6.6 ABI 破坏: mm_struct 6.6 冻结边界漂移");
static_assert(offsetof(struct mm_struct, context) >= 0x4c0 &&
	      offsetof(struct mm_struct, vma_writer_wait) >= 0x4c0 &&
	      offsetof(struct mm_struct, futex_hash_lock) >= 0x4c0 &&
	      offsetof(struct mm_struct, iommu_mm) >= 0x4c0 &&
	      offsetof(struct mm_struct, mm_id) >= 0x4c0 &&
	      offsetof(struct mm_struct, android_vendor_data1) >= 0x4c0,
	      "rodin 6.6 ABI 破坏: mm_struct 6.18 真身成员侵入冻结区");
