// SPDX-License-Identifier: GPL-2.0-only
/*
 * rodin 6.6-compat: memory relinquish service (port of 6.6
 * arch/arm64/mm/mem_relinquish.c).  6.18 dropped the framework; vendor
 * virtio_balloon.ko imports the entry points.  On hardware without the KVM
 * hypervisor services (this device) the behaviour is identical to 6.6:
 * page_relinquish() is a no-op.
 */
#include <linux/arm-smccc.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/types.h>

#include <asm/hypervisor.h>

#ifndef ARM_SMCCC_KVM_FUNC_HYP_MEMINFO
#define ARM_SMCCC_KVM_FUNC_HYP_MEMINFO		2

#define ARM_SMCCC_VENDOR_HYP_KVM_HYP_MEMINFO_FUNC_ID			\
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL,				\
			   ARM_SMCCC_SMC_64,				\
			   ARM_SMCCC_OWNER_VENDOR_HYP,			\
			   ARM_SMCCC_KVM_FUNC_HYP_MEMINFO)
#endif

#ifndef ARM_SMCCC_KVM_FUNC_MEM_RELINQUISH
#define ARM_SMCCC_KVM_FUNC_MEM_RELINQUISH	9

#define ARM_SMCCC_VENDOR_HYP_KVM_MEM_RELINQUISH_FUNC_ID			\
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL,				\
			   ARM_SMCCC_SMC_64,				\
			   ARM_SMCCC_OWNER_VENDOR_HYP,			\
			   ARM_SMCCC_KVM_FUNC_MEM_RELINQUISH)
#endif

bool page_relinquish_disallowed(void);
void page_relinquish(struct page *page);
void post_page_relinquish_tlb_inv(void);

static bool __page_relinquish_disallowed;
static unsigned long memshare_granule_sz;

static void kvm_page_relinquish(struct page *page)
{
	phys_addr_t phys, end;
	u32 func_id = ARM_SMCCC_VENDOR_HYP_KVM_MEM_RELINQUISH_FUNC_ID;

	phys = page_to_phys(page);
	end = phys + PAGE_SIZE;

	while (phys < end) {
		struct arm_smccc_res res;

		arm_smccc_1_1_invoke(func_id, phys, 0, 0, &res);
		BUG_ON(res.a0 != SMCCC_RET_SUCCESS);

		phys += memshare_granule_sz;
	}
}

static void (*hyp_page_relinquish)(struct page *page);

static int __init kvm_init_memrelinquish_services(void)
{
	int i;
	struct arm_smccc_res res;
	const u32 funcs[] = {
		ARM_SMCCC_KVM_FUNC_HYP_MEMINFO,
		ARM_SMCCC_KVM_FUNC_MEM_RELINQUISH,
	};

	for (i = 0; i < ARRAY_SIZE(funcs); ++i) {
		if (!kvm_arm_hyp_service_available(funcs[i]))
			return 0;
	}

	arm_smccc_1_1_invoke(ARM_SMCCC_VENDOR_HYP_KVM_HYP_MEMINFO_FUNC_ID,
			     0, 0, 0, &res);
	if (res.a0 > PAGE_SIZE) { /* Includes error codes */
		__page_relinquish_disallowed = true;
		return 0;
	}

	memshare_granule_sz = res.a0;

	if (memshare_granule_sz)
		hyp_page_relinquish = kvm_page_relinquish;
	return 0;
}
arch_initcall(kvm_init_memrelinquish_services);

bool page_relinquish_disallowed(void)
{
	return __page_relinquish_disallowed;
}
EXPORT_SYMBOL_GPL(page_relinquish_disallowed);

void page_relinquish(struct page *page)
{
	if (hyp_page_relinquish)
		hyp_page_relinquish(page);
}
EXPORT_SYMBOL_GPL(page_relinquish);

void post_page_relinquish_tlb_inv(void)
{
	/* 6.6: only the KVM hyp_ops backend had a hook; none without pKVM. */
}
EXPORT_SYMBOL_GPL(post_page_relinquish_tlb_inv);
