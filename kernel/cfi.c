// SPDX-License-Identifier: GPL-2.0
/*
 * Clang Control Flow Integrity (CFI) error handling.
 *
 * Copyright (C) 2022 Google LLC
 */

#include <linux/bpf.h>
#include <linux/cfi_types.h>
#include <linux/cfi.h>

bool cfi_warn __ro_after_init = IS_ENABLED(CONFIG_CFI_PERMISSIVE);

/* rodin: 逐站点普查的站点表容量 */
#define RODIN_CFI_MAX_SITES	256

enum bug_trap_type report_cfi_failure(struct pt_regs *regs, unsigned long addr,
				      unsigned long *target, u32 type)
{
	/*
	 * rodin: 预编译 6.6 模块的回调带 6.6 的 KCFI 类型哈希，与 6.18 的类型
	 * 哈希天然不同，每次间接调用都会触发。
	 *
	 * 这里是**逐站点普查**：同一个 brk 站点只打一次完整行（含整栈），最多
	 * 256 个站点，之后回退到限流。r13 真机上限 60 行/分钟被 of_iommu_xlate
	 * 一个站点打满，后面的漂移点被整条吃掉 —— 逐站点计数保证一次启动就能把
	 * 全部间接调用漂移点枚举干净。
	 */
	static unsigned long rodin_cfi_sites[RODIN_CFI_MAX_SITES];
	static atomic_t rodin_cfi_sites_nr = ATOMIC_INIT(0);
	static DEFINE_RATELIMIT_STATE(cfi_rs, 60 * HZ, 60);
	bool census_hit = false;
	int nr, i;

	if (cfi_warn) {
		nr = atomic_read(&rodin_cfi_sites_nr);
		for (i = 0; i < nr && i < RODIN_CFI_MAX_SITES; i++)
			if (rodin_cfi_sites[i] == addr)
				break;
		if (i == nr && nr < RODIN_CFI_MAX_SITES) {
			rodin_cfi_sites[nr] = addr;
			atomic_inc(&rodin_cfi_sites_nr);
			census_hit = true;
		}
	}

	if (!census_hit && !__ratelimit(&cfi_rs))
		return BUG_TRAP_TYPE_WARN;

	if (target)
		pr_err("CFI failure at %pS (target: %pS; expected type: 0x%08x)\n",
		       (void *)addr, (void *)*target, type);
	else
		pr_err("CFI failure at %pS (no target information)\n",
		       (void *)addr);

	if (cfi_warn) {
		if (census_hit)
			__warn(NULL, 0, (void *)addr, 0, regs, NULL);
		return BUG_TRAP_TYPE_WARN;
	}

	return BUG_TRAP_TYPE_BUG;
}

/*
 * Declare two non-existent functions with types that match bpf_func_t and
 * bpf_callback_t pointers, and use DEFINE_CFI_TYPE to define type hash
 * variables for each function type. The cfi_bpf_* variables are used by
 * arch-specific BPF JIT implementations to ensure indirectly callable JIT
 * code has matching CFI type hashes.
 */
extern typeof(*(bpf_func_t)0) __bpf_prog_runX;
DEFINE_CFI_TYPE(cfi_bpf_hash, __bpf_prog_runX);

extern typeof(*(bpf_callback_t)0) __bpf_callback_fn;
DEFINE_CFI_TYPE(cfi_bpf_subprog_hash, __bpf_callback_fn);

#ifdef CONFIG_ARCH_USES_CFI_TRAPS
static inline unsigned long trap_address(s32 *p)
{
	return (unsigned long)((long)p + (long)*p);
}

static bool is_trap(unsigned long addr, s32 *start, s32 *end)
{
	s32 *p;

	for (p = start; p < end; ++p) {
		if (trap_address(p) == addr)
			return true;
	}

	return false;
}

#ifdef CONFIG_MODULES
/* Populates `kcfi_trap(_end)?` fields in `struct module`. */
void module_cfi_finalize(const Elf_Ehdr *hdr, const Elf_Shdr *sechdrs,
			 struct module *mod)
{
	char *secstrings;
	unsigned int i;

	mod->kcfi_traps = NULL;
	mod->kcfi_traps_end = NULL;

	secstrings = (char *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;

	for (i = 1; i < hdr->e_shnum; i++) {
		if (strcmp(secstrings + sechdrs[i].sh_name, "__kcfi_traps"))
			continue;

		mod->kcfi_traps = (s32 *)sechdrs[i].sh_addr;
		mod->kcfi_traps_end = (s32 *)(sechdrs[i].sh_addr + sechdrs[i].sh_size);
		break;
	}
}

static bool is_module_cfi_trap(unsigned long addr)
{
	struct module *mod;
	bool found = false;

	guard(rcu)();
	mod = __module_address(addr);
	if (mod)
		found = is_trap(addr, mod->kcfi_traps, mod->kcfi_traps_end);

	return found;
}
#else /* CONFIG_MODULES */
static inline bool is_module_cfi_trap(unsigned long addr)
{
	return false;
}
#endif /* CONFIG_MODULES */

extern s32 __start___kcfi_traps[];
extern s32 __stop___kcfi_traps[];

bool is_cfi_trap(unsigned long addr)
{
	if (is_trap(addr, __start___kcfi_traps, __stop___kcfi_traps))
		return true;

	return is_module_cfi_trap(addr);
}
#endif /* CONFIG_ARCH_USES_CFI_TRAPS */
