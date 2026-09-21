// SPDX-License-Identifier: GPL-2.0
#include <linux/bug.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <net/dropreason-core.h>

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
