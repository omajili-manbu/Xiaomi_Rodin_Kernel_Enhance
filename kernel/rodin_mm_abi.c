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
 * nodeinfo[nid] 取 per-node 对象 */
RODIN_MM_OFF(struct mem_cgroup, android_oem_data1, 0x8f0);
RODIN_MM_OFF(struct mem_cgroup, nodeinfo, 0x900);

/* mem_cgroup_per_node：xswapd 按 6.6 lruvec.pgdat 偏移同步回指针 */
RODIN_MM_OFF(struct mem_cgroup_per_node, rodin_lruvec_pgdat, 0x638);

/* node_stat_item：blocktag/mpbe 按 6.6 枚举序的编译期下标 */
static_assert(NR_FILE_PAGES == 19);
static_assert(NR_INACTIVE_ANON == 0 && NR_ACTIVE_ANON == 1 &&
	      NR_INACTIVE_FILE == 2);
