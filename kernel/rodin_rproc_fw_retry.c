// SPDX-License-Identifier: GPL-2.0
/*
 * rodin A-47: vendor remoteproc 固件晚绑定 boot 重试原语（§108 就绪原语族②）
 *
 * 6.6 时代 vendor remoteproc 驱动（mtk_ccd 等）=m，由 insmod 在 /vendor
 * 挂载后装载，probe 内的 auto_boot 一次成功；6.18 =y 内建后 probe 落在
 * initcall 期，早于 /vendor 挂载，而 remoteproc 核心的 auto_boot 只打一发
 * request_firmware_nowait，失败后无人重试（#178 真机 4.993s remoteproc-ccd
 * 固件 -2 x2 后 CCD 未 boot）。
 *
 * 本原语不进 remoteproc 核心、不改 vendor 驱动：late_initcall_sync 后按
 * DT compatible 定位 rproc 子设备，轮询 rproc_boot（核心失败路径 downref
 * 干净可重试）直到固件随 /vendor 挂载可得。发现链 = compatible 节点 ->
 * platform 设备 -> 名为 remoteprocN 的子设备（核心 dev_set_name，
 * remoteproc_core.c dev_set_name(&rproc->dev, "remoteproc%d")），
 * container_of 后以 rproc->name 复核。
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#define RODIN_CCD_COMPATIBLE		"mediatek,ccd"
#define RODIN_CCD_RPROC_NAME		"remoteproc-ccd"
#define RODIN_RPROC_BOOT_FIRST_DELAY_MS	2000
#define RODIN_RPROC_BOOT_RETRY_MS	1000
#define RODIN_RPROC_BOOT_MAX_ATTEMPTS	30

static struct delayed_work rodin_ccd_boot_work;
static struct rproc *rodin_ccd_rproc;
static int rodin_ccd_boot_attempts;

static int rodin_rproc_child_match(struct device *dev, const void *data)
{
	return str_has_prefix(dev_name(dev), "remoteproc") != 0;
}

static struct rproc *rodin_ccd_rproc_find(void)
{
	struct device_node *np;
	struct platform_device *pdev;
	struct device *dev;
	struct rproc *rproc;

	np = of_find_compatible_node(NULL, NULL, RODIN_CCD_COMPATIBLE);
	if (!np)
		return NULL;

	pdev = of_find_device_by_node(np);
	of_node_put(np);
	if (!pdev)
		return NULL;

	dev = device_find_child(&pdev->dev, NULL, rodin_rproc_child_match);
	put_device(&pdev->dev);
	if (!dev)
		return NULL;

	rproc = container_of(dev, struct rproc, dev);
	if (strcmp(rproc->name, RODIN_CCD_RPROC_NAME)) {
		put_device(dev);
		return NULL;
	}

	/*
	 * 持住 device 引用不放：内建单例，ccd_remove 终不运行，
	 * 后续重试轮次还要继续用该 rproc。
	 */
	return rproc;
}

static void rodin_ccd_boot_fn(struct work_struct *work)
{
	struct rproc *rproc = READ_ONCE(rodin_ccd_rproc);
	int ret = 0;

	if (!rproc) {
		rproc = rodin_ccd_rproc_find();
		if (!rproc)
			goto retry;
		WRITE_ONCE(rodin_ccd_rproc, rproc);
	}

	if (rproc->state == RPROC_RUNNING) {
		dev_info(&rproc->dev, "rodin: ccd already running, no retry needed\n");
		return;
	}

	ret = rproc_boot(rproc);
	if (!ret) {
		dev_info(&rproc->dev, "rodin: ccd firmware booted after %d retried attempt(s)\n",
			 rodin_ccd_boot_attempts);
		return;
	}

retry:
	rodin_ccd_boot_attempts++;
	if (rodin_ccd_boot_attempts >= RODIN_RPROC_BOOT_MAX_ATTEMPTS) {
		if (rproc)
			dev_info(&rproc->dev,
				 "rodin: ccd firmware boot gave up after %d attempts (last error %d)\n",
				 rodin_ccd_boot_attempts, ret);
		else
			pr_info("ccd rproc never found, gave up after %d attempts\n",
				rodin_ccd_boot_attempts);
		return;
	}

	pr_info("ccd firmware not ready (attempt %d/%d, error %d), retrying\n",
		rodin_ccd_boot_attempts, RODIN_RPROC_BOOT_MAX_ATTEMPTS, ret);
	schedule_delayed_work(&rodin_ccd_boot_work,
			      msecs_to_jiffies(RODIN_RPROC_BOOT_RETRY_MS));
}

static int __init rodin_rproc_fw_retry_init(void)
{
	INIT_DELAYED_WORK(&rodin_ccd_boot_work, rodin_ccd_boot_fn);
	schedule_delayed_work(&rodin_ccd_boot_work,
			      msecs_to_jiffies(RODIN_RPROC_BOOT_FIRST_DELAY_MS));
	return 0;
}
late_initcall_sync(rodin_rproc_fw_retry_init);
