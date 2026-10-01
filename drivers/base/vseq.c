// SPDX-License-Identifier: GPL-2.0
/*
 * VSEQ runner —— 按原厂装载序重放 vendor ex-module 的 module_init 族。
 * 挂点：do_basic_setup() 末尾（do_initcalls 全部完成之后、free_initmem 之前）。
 * 顺序依据：drivers/base/vseq_order_gen.c（= 6.6 真机实测 224 条装载序）。
 * 可观测性：每个模块一条 "vseq: [n] <mod>"，失败条目 pr_warn 返回值；
 * 挂死时最后一行即肇事模块（bootprof 等价物）。
 */
#include <linux/vseq.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/printk.h>
#include <linux/ctype.h>

extern const struct vseq_ent __vseq_entries_start[];
extern const struct vseq_ent __vseq_entries_end[];

#define VSEQ_SEQ_UNKNOWN UINT_MAX

/* gen_vseq.py norm() 同规则：-/ _ 等价、小写 */
static unsigned int vseq_seq_of(const char *mod)
{
	char buf[64];
	size_t i, j;

	for (i = 0, j = 0; mod[i] && j < sizeof(buf) - 1; i++) {
		char c = mod[i];
		if (c == '-')
			c = '_';
		buf[j++] = (char)tolower((unsigned char)c);
	}
	buf[j] = '\0';

	for (i = 0; i < __vseq_mod_order_nr; i++) {
		if (!strcmp(buf, __vseq_mod_order[i].name))
			return __vseq_mod_order[i].seq;
	}
	return VSEQ_SEQ_UNKNOWN;
}

struct vseq_idx {
	const struct vseq_ent *e;
	unsigned int seq;
	size_t ord;
};

/* 稳定序：seq 优先，同 seq 按段内原序（heapsort 非稳定，用 ord 兜底） */
static int vseq_cmp(const void *a, const void *b)
{
	const struct vseq_idx *x = a, *y = b;

	if (x->seq != y->seq)
		return x->seq < y->seq ? -1 : 1;
	if (x->ord != y->ord)
		return x->ord < y->ord ? -1 : 1;
	return 0;
}

void __init vseq_replay(void)
{
	size_t n = __vseq_entries_end - __vseq_entries_start;
	struct vseq_idx *idx;
	size_t i;
	unsigned int cur_seq = VSEQ_SEQ_UNKNOWN, pos = 0;
	int ret;

	if (!n || !__vseq_mod_order_nr)
		return;

	idx = kmalloc_array(n, sizeof(idx[0]), GFP_KERNEL);
	if (!idx) {
		pr_err("vseq: OOM, %zu entries skipped\n", n);
		return;
	}

	for (i = 0; i < n; i++) {
		idx[i].e = &__vseq_entries_start[i];
		idx[i].seq = vseq_seq_of(idx[i].e->mod);
		idx[i].ord = i;
	}
	sort(idx, n, sizeof(idx[0]), vseq_cmp, NULL);

	pr_info("vseq: replaying %zu init entries in stock module load order\n", n);
	for (i = 0; i < n; i++) {
		const struct vseq_ent *e = idx[i].e;

		if (idx[i].seq != cur_seq) {
			cur_seq = idx[i].seq;
			if (cur_seq == VSEQ_SEQ_UNKNOWN)
				pr_warn("vseq: %s not in order table, running at tail\n", e->mod);
			else
				pr_info("vseq: [%3u] %s\n", ++pos, e->mod);
		}
		ret = e->fn();
		if (ret)
			pr_warn("vseq: %s: init returned %d\n", e->mod, ret);
	}
	pr_info("vseq: replay done, %u modules\n", pos);
	kfree(idx);
}
