// SPDX-License-Identifier: GPL-2.0
#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/maple_tree.h>
#include <linux/stddef.h>
#include <linux/printk.h>
#include <net/dropreason-core.h>
#include <linux/io.h>
#include <asm/io.h>

/* 6.18 turned fortify_panic into a macro; our 6.6-shaped one comes below */
#ifdef fortify_panic
#undef fortify_panic
#endif

#ifdef CONFIG_MODULE_FORCE_LOAD

struct seq_file;

void __seq_puts(struct seq_file *m, const char *s);
void seq_puts(struct seq_file *m, const char *s);
void fortify_panic(const char *name) __cold __noreturn;
struct sk_buff;
struct sock;
void sk_skb_reason_drop(struct sock *sk, struct sk_buff *skb,
			enum skb_drop_reason reason);
void kfree_skb_reason(struct sk_buff *skb, enum skb_drop_reason reason);

void seq_puts(struct seq_file *m, const char *s)
{
	__seq_puts(m, s);
}
EXPORT_SYMBOL(seq_puts);

void kfree_skb_reason(struct sk_buff *skb, enum skb_drop_reason reason)
{
	sk_skb_reason_drop(NULL, skb, reason);
}
EXPORT_SYMBOL(kfree_skb_reason);

#undef fortify_panic

void fortify_panic(const char *name)
{
	pr_emerg("detected buffer overflow in %s\n", name);
	BUG();
}
EXPORT_SYMBOL(fortify_panic);

#endif

/* ---- arm64 io-memory primitives renamed in 6.18 (lib/iomem_copy.c) ---- */

void __memset_io(volatile void __iomem *dst, int c, size_t count);
void __memcpy_toio(volatile void __iomem *to, const void *from, size_t count);
void __memcpy_fromio(void *to, const volatile void __iomem *from, size_t count);


void __memset_io(volatile void __iomem *dst, int c, size_t count)
{
	memset_io(dst, c, count);
}
EXPORT_SYMBOL(__memset_io);

void __memcpy_toio(volatile void __iomem *to, const void *from, size_t count)
{
	memcpy_toio(to, from, count);
}
EXPORT_SYMBOL(__memcpy_toio);

void __memcpy_fromio(void *to, const volatile void __iomem *from, size_t count)
{
	memcpy_fromio(to, from, count);
}
EXPORT_SYMBOL(__memcpy_fromio);


/* ---- 6.6 blob 的 ma_state：MAS_START 哨兵与 6.18 status 状态机（#202/#203）----
 *
 * 6.18 maple tree 重构后 struct ma_state 从 64B 长到 88B，状态机改由新增的
 * status@72 驱动，MAS_START/MAS_NONE/MAS_PAUSE 哨兵废除（MA_STATE 初始化为
 * node=NULL + status=ma_start）。6.6 编译的 blob 以 VMA_ITERATOR/MA_STATE
 * 内联初始化，只写前 64B 并置 node=MAS_START=((void *)1UL)（6.6
 * maple_tree.h:434），64B 之外的 status/depth/offset/mas_flags/end/
 * store_type 全是栈垃圾：mas_find_setup() 对垃圾 status 无 case 命中，
 * mas_is_start() 为假，带着 node=1 直进 mas_next_slot()，mte_to_node(1)=0
 * 解引用 NULL（#201 6.927s，T867 keymint TEE_IOC_SHM_REGISTER ->
 * mitee optee_check_mem_type -> mas_find -> mas_next_slot+0x54）。
 *
 * #202 的原地重初始化被真机否决（A-61）：blob 的 64B 是栈上预留，6.18 任何
 * maple 入口——包装器补尾字段、真 mas_find 收尾的 status=ma_active——都要
 * 写 64..87；mitee optee_check_mem_type 的 canary 紧贴状态槽（sp+0x48，其上
 * 还有保存的 x29/x30），首调 shm register 即 7.076s __stack_chk_fail。写界
 * 必须整体缩回 64B 内。
 *
 * 方案S2（无影子表）：只读 blob 前缀 tree/index/last（0..23，与 6.18 同形），
 * 在包装器自己的栈上按 6.18 MA_STATE 语义新建 88B 影子态跑真 mas_find，回拷
 * 只写 last@16 与 index@8。续走 = index 锚驱动重找：maple 每槽一项且项区间
 * 不相交，回拷 index = 槽位窗顶 s.max+1（上一条目区间端 +1），下一次从该
 * index 重找与 6.6 的 node 续走等价；blob 迭代间不读不改 state（mitee 反汇
 * 编实证，唯循环读返回 vma），即使重播种也必前进、不回吐同一条目。
 * ULONG_MAX 槽顶不 +1 防回卷（此时下一调 index=max 命中 mas_find_setup 的
 * index>max 提前 NULL，max<ULONG_MAX 时成立）。文档化的等价边界：0 长条目
 * 与 NULL 后换更大 max 续调不在等价范围；活体 blob 全查仅 mitee 一处
 * mas_find（monitor_hang/mtk_heap_debug 已内建接管，gzvm 走内核自建 state），
 * VMA 树两条边界均不存在。
 * 只服务 mod_6_6 blob（kernel/module/main.c simplify_symbols() 重定向）。
 * 门：noinline 独立可断言；static_assert 钉死漂移面与写界（回拷只落
 * index@8/last@16，均在 6.6 预留的 0..63 内），头文件再动先过人。
 */

void *mas_find(struct ma_state *mas, unsigned long max);
void *rodin_mas_find_6_6(struct ma_state *mas, unsigned long max);

static_assert(sizeof(struct ma_state) == 88);
/* 读写面：blob 6.6 前缀 tree@0/index@8/last@16 与 6.18 同形（mitee 反汇编实证） */
static_assert(offsetof(struct ma_state, tree) == 0);
static_assert(offsetof(struct ma_state, index) == 8);
static_assert(offsetof(struct ma_state, last) == 16);
/* 6.18 漂移面：sheaf@48/alloc@56 仍落 blob 64B 预留内（影子设计不碰它们），
 * node_request@64 起才是 6.6 预留外的越界区——#202 原地重初始化即写穿此处 */
static_assert(offsetof(struct ma_state, sheaf) == 48);
static_assert(offsetof(struct ma_state, alloc) == 56);
static_assert(offsetof(struct ma_state, node_request) == 64);
static_assert(offsetof(struct ma_state, status) == 72);

noinline void *rodin_mas_find_6_6(struct ma_state *mas, unsigned long max)
{
	/* 6.18 MA_STATE 同形初始化；depth/offset/end 由指定初始化补零 */
	struct ma_state s = {
		.tree = mas->tree,
		.index = mas->index,
		.last = mas->last,
		.node = NULL,
		.status = ma_start,
		.min = 0,
		.max = ULONG_MAX,
		.sheaf = NULL,
		.alloc = NULL,
		.node_request = 0,
		.mas_flags = 0,
		.store_type = wr_invalid,
	};
	void *entry = mas_find(&s, max);

	if (entry) {
		mas->last = s.last;
		mas->index = (s.max == ULONG_MAX) ? s.max : s.max + 1;
	}

	return entry;
}
EXPORT_SYMBOL(rodin_mas_find_6_6);
