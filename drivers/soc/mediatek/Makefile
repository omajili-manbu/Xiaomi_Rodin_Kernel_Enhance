# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_MTK_CMDQ) += mtk-cmdq-helper.o
obj-$(CONFIG_MTK_DEVAPC) += mtk-devapc.o
# rodin stage2: kernel-side DVFSRC (MT8183/8192/8195 only) disabled -- the
# vendor mtk-dvfsrc.o builds from the same CONFIG_MTK_DVFSRC symbol and is the
# implementation rodin (mt6899) actually uses; both would collide at link.
#obj-$(CONFIG_MTK_DVFSRC) += mtk-dvfsrc.o
obj-$(CONFIG_MTK_INFRACFG) += mtk-infracfg.o
obj-$(CONFIG_MTK_PMIC_WRAP) += mtk-pmic-wrap.o
obj-$(CONFIG_MTK_REGULATOR_COUPLER) += mtk-regulator-coupler.o
obj-$(CONFIG_MTK_MMSYS) += mtk-mmsys.o
obj-$(CONFIG_MTK_MMSYS) += mtk-mutex.o
obj-$(CONFIG_MTK_SVS) += mtk-svs.o
obj-$(CONFIG_MTK_SOCINFO) += mtk-socinfo.o
