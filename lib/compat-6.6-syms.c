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


/* ---- 6.6 blob 的 ma_state：MAS_START 哨兵与 6.18 status 状态机（#202）----
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
 * node==1 在 6.18 不可能是合法中间态：合法 enode 是 8 对齐节点指针低位
 * or 节点类型 tag（mte_to_node 按 ~7 掩码解码），或 NULL——判定精确。
 * 只服务 mod_6_6 blob（kernel/module/main.c simplify_symbols() 重定向）；
 * 重初始化按 6.18 MA_STATE 语义补齐全部尾字段，保 tree/index/last
 * （last=0 与内核自身 VMA_ITERATOR 同形）。首调之后 state 归 6.18 状态机。
 * 门：noinline 独立可断言；static_assert 钉死漂移面，头文件再动先过人。
 */

void *mas_find(struct ma_state *mas, unsigned long max);
void *rodin_mas_find_6_6(struct ma_state *mas, unsigned long max);

static_assert(sizeof(struct ma_state) == 88);
static_assert(offsetof(struct ma_state, node) == 24);
static_assert(offsetof(struct ma_state, status) == 72);

noinline void *rodin_mas_find_6_6(struct ma_state *mas, unsigned long max)
{
	if (unlikely((unsigned long)mas->node == 1UL)) {
		mas->node = NULL;
		mas->status = ma_start;
		mas->min = 0;
		mas->max = ULONG_MAX;
		mas->sheaf = NULL;
		mas->alloc = NULL;
		mas->node_request = 0;
		mas->depth = 0;
		mas->offset = 0;
		mas->mas_flags = 0;
		mas->end = 0;
		mas->store_type = wr_invalid;
	}

	return mas_find(mas, max);
}
EXPORT_SYMBOL(rodin_mas_find_6_6);
