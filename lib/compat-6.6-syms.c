// SPDX-License-Identifier: GPL-2.0
#include <linux/bug.h>
#include <linux/export.h>
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
