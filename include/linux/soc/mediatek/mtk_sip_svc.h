/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 */
#ifndef __MTK_SIP_SVC_H
#define __MTK_SIP_SVC_H

/* Error Code */
#define SIP_SVC_E_SUCCESS               0
#define SIP_SVC_E_NOT_SUPPORTED         -1
#define SIP_SVC_E_INVALID_PARAMS        -2
#define SIP_SVC_E_INVALID_RANGE         -3
#define SIP_SVC_E_PERMISSION_DENIED     -4

#ifdef CONFIG_ARM64
#define MTK_SIP_SMC_CONVENTION          ARM_SMCCC_SMC_64
#else
#define MTK_SIP_SMC_CONVENTION          ARM_SMCCC_SMC_32
#endif

#define MTK_SIP_SMC_CMD(fn_id) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, MTK_SIP_SMC_CONVENTION, \
			   ARM_SMCCC_OWNER_SIP, fn_id)

/* DVFSRC SMC calls */
#define MTK_SIP_DVFSRC_VCOREFS_CONTROL	MTK_SIP_SMC_CMD(0x506)

/* IOMMU related SMC call */
#define MTK_SIP_KERNEL_IOMMU_CONTROL	MTK_SIP_SMC_CMD(0x514)
/* rodin r25: vendor sources (iommu_secure.c) call the same id by this
 * vendor-side name; alias instead of duplicating the id. */
#define MTK_IOMMU_SECURE_CONTROL		MTK_SIP_KERNEL_IOMMU_CONTROL


/* rodin r25: vendor SMC ids (from MTK_kernel_device_modules
 * include/linux/soc/mediatek/mtk_sip_svc.h). The 6.18 upstream header resolves
 * first for the builtin vendor modules and lacked them; values are MTK's. */
#ifndef MTK_SIP_VCOREFS_CONTROL
#define MTK_SIP_VCOREFS_CONTROL		MTK_SIP_SMC_CMD(0x506)
#endif
#ifndef MTK_SIP_KERNEL_DEVMPU_VIO_GET
#define MTK_SIP_KERNEL_DEVMPU_VIO_GET	MTK_SIP_SMC_CMD(0x264)
#endif
#ifndef MTK_SIP_KERNEL_DEVMPU_PERM_GET
#define MTK_SIP_KERNEL_DEVMPU_PERM_GET	MTK_SIP_SMC_CMD(0x265)
#endif
#ifndef MTK_SIP_KERNEL_DEVMPU_VIO_CLR
#define MTK_SIP_KERNEL_DEVMPU_VIO_CLR	MTK_SIP_SMC_CMD(0x268)
#endif
#ifndef MTK_SIP_KERNEL_DAPC_PERM_GET
#define MTK_SIP_KERNEL_DAPC_PERM_GET	MTK_SIP_SMC_CMD(0x26B)
#endif
#ifndef MTK_SIP_KERNEL_CLR_SRAMROM_VIO
#define MTK_SIP_KERNEL_CLR_SRAMROM_VIO	MTK_SIP_SMC_CMD(0x26C)
#endif
#ifndef MTK_SIP_TINYSYS_VCP_CONTROL
#define MTK_SIP_TINYSYS_VCP_CONTROL	MTK_SIP_SMC_CMD(0x52C)
#endif
#ifndef MTK_SIP_KERNEL_DAPC_MMUP_CONTROL
#define MTK_SIP_KERNEL_DAPC_MMUP_CONTROL	MTK_SIP_SMC_CMD(0x52E)
#endif
#ifndef MTK_SIP_KERNEL_DAPC_SUBSYS_GET
#define MTK_SIP_KERNEL_DAPC_SUBSYS_GET	MTK_SIP_SMC_CMD(0x531)
#endif
#ifndef MTK_SIP_KERNEL_RGU_CONTROL
#define MTK_SIP_KERNEL_RGU_CONTROL	MTK_SIP_SMC_CMD(0x53A)
#endif
#ifndef MTK_SIP_TINYSYS_SSPM_CONTROL
#define MTK_SIP_TINYSYS_SSPM_CONTROL	MTK_SIP_SMC_CMD(0x53C)
#endif
#ifndef MTK_SIP_KERNEL_HWCCF_CONTROL
#define MTK_SIP_KERNEL_HWCCF_CONTROL	MTK_SIP_SMC_CMD(0x540)
#endif
#ifndef MTK_SIP_KERNEL_WDT
#define MTK_SIP_KERNEL_WDT		MTK_SIP_SMC_CMD(0x200)
#endif
#ifndef MTK_SIP_KERNEL_USB_CONTROL
/* rodin stage2: vendor SIP IDs needed by built-in vendor drivers
 * (values from the 6.6 vendor header; upstream keeps a smaller set) */
#define MTK_SIP_KERNEL_USB_CONTROL	MTK_SIP_SMC_CMD(0x527)
#endif
#ifndef MTK_SIP_KERNEL_SPI_CONTROL
#define MTK_SIP_KERNEL_SPI_CONTROL	MTK_SIP_SMC_CMD(0x542)
#endif
#ifndef MTK_SIP_CMDQ_CONTROL
/* rodin stage2 batch4: vendor SIP ID needed by cmdq mailbox ext */
#define MTK_SIP_CMDQ_CONTROL	MTK_SIP_SMC_CMD(0x518)
#endif
#endif
