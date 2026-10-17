// SPDX-License-Identifier: GPL-2.0-only
/*
 * MTD Oops/Panic logger
 *
 * Copyright © 2007 Nokia Corporation. All rights reserved.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/vseq.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/timekeeping.h>
#include <linux/mtd/mtd.h>
#include <linux/kmsg_dump.h>
#include <linux/reboot.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/io.h>

/* Maximum MTD partition size */
#define MTDOOPS_MAX_MTD_SIZE (16 * 1024 * 1024)

static unsigned long record_size = 4096;
module_param(record_size, ulong, 0400);
MODULE_PARM_DESC(record_size,
		"record size for MTD OOPS pages in bytes (default 4096)");

static char mtddev[80];
module_param_string(mtddev, mtddev, 80, 0400);
MODULE_PARM_DESC(mtddev,
		"name or index number of the MTD device to use");

static int dump_oops = 1;
module_param(dump_oops, int, 0600);
MODULE_PARM_DESC(dump_oops,
		"set to 1 to dump oopses, 0 to only dump panics (default 1)");

#define MTDOOPS_KERNMSG_MAGIC_v1 0x5d005d00  /* Original */
#define MTDOOPS_KERNMSG_MAGIC_v2 0x5d005e00  /* Adds the timestamp */
#define MTDOOPS_HEADER_SIZE   8

enum mtd_dump_reason {
	MTD_DUMP_UNDEF,
	MTD_DUMP_PANIC,
	MTD_DUMP_OOPS,
	MTD_DUMP_EMERG,
	MTD_DUMP_SHUTDOWN,
	MTD_DUMP_RESTART,
	MTD_DUMP_POWEROFF,
	MTD_DUMP_LONG_PRESS,
	MTD_DUMP_MAX
};

static const char * const kdump_reason[MTD_DUMP_MAX] = {
	"Unknown",
	"Kernel Panic",
	"Oops!",
	"Emerg",
	"Shut Down",
	"Restart",
	"PowerOff",
	"Long Press"
};

enum mtdoops_log_type {
	MTDOOPS_TYPE_UNDEF,
	MTDOOPS_TYPE_DMESG,
	MTDOOPS_TYPE_PMSG,
};

static const char * const log_type[3] = {
	"Unknown",
	"LAST KMSG",
	"LAST LOGCAT"
};

struct pmsg_buffer_hdr {
	uint32_t    sig;
	atomic_t    start;
	atomic_t    size;
	uint8_t     data[];
};

/* ramoops pmsg window signature ("DBGC") */
#define MTDOOPS_PMSG_SIG	0x43474244

/* upper bound for the "LAST LOGCAT" marker written ahead of the payload */
#define MTDOOPS_PMSG_HEAD_MAX	80

/* slot reserved for the reason banner in front of the kmsg text */
#define MTDOOPS_BANNER_MAX	544

struct mtdoops_hdr {
	u32 seq;
	u32 magic;
	ktime_t timestamp;
} __packed;

struct pmsg_platform_data {
	unsigned long	mem_size;
	phys_addr_t	mem_address;
	unsigned long	console_size;
	unsigned long	pmsg_size;
};

static struct mtdoops_context {
	struct kmsg_dumper dump;
	struct notifier_block reboot_nb;
	struct pmsg_platform_data pmsg_data;

	int mtd_index;
	struct work_struct work_erase;
	struct mtd_info *mtd;
	int oops_pages;
	int nextpage;
	int nextcount;
	unsigned long *oops_page_used;

	unsigned long oops_buf_busy;
	void *oops_buf;
} oops_cxt;

static unsigned long lkmsg_record_size = 512 * 1024;

#define MAX_CMDLINE_PARAM_LEN 256
static char build_fingerprint[MAX_CMDLINE_PARAM_LEN];
module_param_string(fingerprint, build_fingerprint, MAX_CMDLINE_PARAM_LEN, 0644);

static int boot_mode;
module_param(boot_mode, int, 0600);
MODULE_PARM_DESC(boot_mode, "boot_mode (default 0)");

static void mark_page_used(struct mtdoops_context *cxt, int page)
{
	set_bit(page, cxt->oops_page_used);
}

static void mark_page_unused(struct mtdoops_context *cxt, int page)
{
	clear_bit(page, cxt->oops_page_used);
}

static int page_is_used(struct mtdoops_context *cxt, int page)
{
	return test_bit(page, cxt->oops_page_used);
}

static int mtdoops_erase_block(struct mtdoops_context *cxt, int offset)
{
	struct mtd_info *mtd = cxt->mtd;
	u32 start_page_offset = mtd_div_by_eb(offset, mtd) * mtd->erasesize;
	u32 start_page = start_page_offset / record_size;
	u32 erase_pages = mtd->erasesize / record_size;
	struct erase_info erase;
	int ret;
	int page;

	erase.addr = offset;
	erase.len = mtd->erasesize;

	ret = mtd_erase(mtd, &erase);
	if (ret) {
		pr_warn("erase of region [0x%llx, 0x%llx] on \"%s\" failed\n",
			(unsigned long long)erase.addr,
			(unsigned long long)erase.len, mtddev);
		return ret;
	}

	/* Mark pages as unused */
	for (page = start_page; page < start_page + erase_pages; page++)
		mark_page_unused(cxt, page);

	return 0;
}

static void mtdoops_erase(struct mtdoops_context *cxt)
{
	struct mtd_info *mtd = cxt->mtd;
	int i = 0, j, ret, mod;

	/* We were unregistered */
	if (!mtd)
		return;

	mod = (cxt->nextpage * record_size) % mtd->erasesize;
	if (mod != 0) {
		cxt->nextpage = cxt->nextpage + ((mtd->erasesize - mod) / record_size);
		if (cxt->nextpage >= cxt->oops_pages)
			cxt->nextpage = 0;
	}

	while ((ret = mtd_block_isbad(mtd, cxt->nextpage * record_size)) > 0) {
badblock:
		pr_warn("bad block at %08lx\n",
			cxt->nextpage * record_size);
		i++;
		cxt->nextpage = cxt->nextpage + (mtd->erasesize / record_size);
		if (cxt->nextpage >= cxt->oops_pages)
			cxt->nextpage = 0;
		if (i == cxt->oops_pages / (mtd->erasesize / record_size)) {
			pr_err("all blocks bad!\n");
			return;
		}
	}

	if (ret < 0) {
		pr_err("mtd_block_isbad failed, aborting\n");
		return;
	}

	for (j = 0, ret = -1; (j < 3) && (ret < 0); j++)
		ret = mtdoops_erase_block(cxt, cxt->nextpage * record_size);

	if (ret >= 0) {
		pr_debug("ready %d, %d\n",
			 cxt->nextpage, cxt->nextcount);
		return;
	}

	if (ret == -EIO) {
		ret = mtd_block_markbad(mtd, cxt->nextpage * record_size);
		if (ret < 0 && ret != -EOPNOTSUPP) {
			pr_err("block_markbad failed, aborting\n");
			return;
		}
	}
	goto badblock;
}

/* Scheduled work - when we can't proceed without erasing a block */
static void mtdoops_workfunc_erase(struct work_struct *work)
{
	struct mtdoops_context *cxt =
			container_of(work, struct mtdoops_context, work_erase);
	mtdoops_erase(cxt);
}

static void mtdoops_inc_counter(struct mtdoops_context *cxt, int panic)
{
	cxt->nextpage++;
	if (cxt->nextpage >= cxt->oops_pages)
		cxt->nextpage = 0;
	cxt->nextcount++;
	if (cxt->nextcount == 0xffffffff)
		cxt->nextcount = 0;

	if (page_is_used(cxt, cxt->nextpage)) {
		pr_debug("not ready %d, %d (erase %s)\n",
			 cxt->nextpage, cxt->nextcount,
			 panic ? "immediately" : "scheduled");
		if (panic) {
			/* In case of panic, erase immediately */
			mtdoops_erase(cxt);
		} else {
			/* Otherwise, schedule work to erase it "nicely" */
			schedule_work(&cxt->work_erase);
		}
	} else {
		pr_debug("ready %d, %d (no erase)\n",
			 cxt->nextpage, cxt->nextcount);
	}
}

static void mtdoops_write(struct mtdoops_context *cxt, int panic)
{
	struct mtd_info *mtd = cxt->mtd;
	size_t retlen;
	struct mtdoops_hdr *hdr;
	int ret;

	if (test_and_set_bit(0, &cxt->oops_buf_busy))
		return;

	/* Add mtdoops header to the buffer */
	hdr = (struct mtdoops_hdr *)cxt->oops_buf;
	hdr->seq = cxt->nextcount;
	hdr->magic = MTDOOPS_KERNMSG_MAGIC_v2;
	hdr->timestamp = ktime_get_real();

	if (panic) {
		ret = mtd_panic_write(mtd, cxt->nextpage * record_size,
				      record_size, &retlen, cxt->oops_buf);
		if (ret == -EOPNOTSUPP) {
			pr_err("Cannot write from panic without panic_write\n");
			goto out;
		}
	} else
		ret = mtd_write(mtd, cxt->nextpage * record_size,
				record_size, &retlen, cxt->oops_buf);

	if (retlen != record_size || ret < 0)
		pr_err("write failure at %ld (%td of %ld written), error %d\n",
		       cxt->nextpage * record_size, retlen, record_size, ret);
	mark_page_used(cxt, cxt->nextpage);
out:
	clear_bit(0, &cxt->oops_buf_busy);
}

static void find_next_position(struct mtdoops_context *cxt)
{
	struct mtd_info *mtd = cxt->mtd;
	struct mtdoops_hdr hdr;
	int ret, page, maxpos = 0;
	u32 maxcount = 0xffffffff;
	size_t retlen;

	for (page = 0; page < cxt->oops_pages; page++) {
		if (mtd_block_isbad(mtd, page * record_size))
			continue;
		/* Assume the page is used */
		mark_page_used(cxt, page);
		ret = mtd_read(mtd, page * record_size, sizeof(hdr),
			       &retlen, (u_char *)&hdr);
		if (retlen != sizeof(hdr) ||
				(ret < 0 && !mtd_is_bitflip(ret))) {
			pr_err("read failure at %ld (%zu of %zu read), err %d\n",
			       page * record_size, retlen, sizeof(hdr), ret);
			continue;
		}

		if (hdr.seq == 0xffffffff && hdr.magic == 0xffffffff)
			mark_page_unused(cxt, page);
		if (hdr.seq == 0xffffffff ||
		    (hdr.magic != MTDOOPS_KERNMSG_MAGIC_v1 &&
		     hdr.magic != MTDOOPS_KERNMSG_MAGIC_v2))
			continue;
		if (maxcount == 0xffffffff) {
			maxcount = hdr.seq;
			maxpos = page;
		} else if (hdr.seq < 0x40000000 && maxcount > 0xc0000000) {
			maxcount = hdr.seq;
			maxpos = page;
		} else if (hdr.seq > maxcount && hdr.seq < 0xc0000000) {
			maxcount = hdr.seq;
			maxpos = page;
		} else if (hdr.seq > maxcount && hdr.seq > 0xc0000000
					&& maxcount > 0x80000000) {
			maxcount = hdr.seq;
			maxpos = page;
		}
	}
	if (maxcount == 0xffffffff) {
		cxt->nextpage = cxt->oops_pages - 1;
		cxt->nextcount = 0;
	}
	else {
		cxt->nextpage = maxpos;
		cxt->nextcount = maxcount;
	}

	mtdoops_inc_counter(cxt, 0);
}

static void mtdoops_add_reason(char *oops_buf, int reason,
			       enum mtdoops_log_type type, int index)
{
	char str_buf[512] = {0};
	struct timespec64 now;
	struct tm ts;
	char temp_buf[32] = {0};
	int temp_len, ret_len;
	const char *mode;
	unsigned long local_time;

	ktime_get_coarse_real_ts64(&now);
	/* title time in UTC+8 */
	local_time = (unsigned long)(now.tv_sec + 8 * 60 * 60);
	time64_to_tm(local_time, 0, &ts);

	switch (boot_mode) {
	case 1:
		mode = "recovery";
		break;
	case 2:
		mode = "poweroff_charger";
		break;
	default:
		mode = "normal";
		break;
	}

	temp_len = snprintf(temp_buf, sizeof(temp_buf),
			    "\n ---mtdoops report start--- \n");
	memcpy(oops_buf, temp_buf, temp_len);

	ret_len = snprintf(str_buf, sizeof(str_buf),
			   "\n```\n## Oops_Index: %d\n### Build: %s\n## REASON: %s\n#### LOG TYPE:%s\n## BOOT MODE:%s\n##### %04ld-%02d-%02d %02d:%02d:%02d\n```c\n",
			   index, build_fingerprint,
			   reason < MTD_DUMP_MAX ? kdump_reason[reason] : "Unknown",
			   type < ARRAY_SIZE(log_type) ? log_type[type] : "Unknown",
			   mode,
			   ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
			   ts.tm_hour, ts.tm_min, ts.tm_sec);

	if (ret_len >= sizeof(str_buf))
		ret_len = sizeof(str_buf) - 1;

	memcpy(oops_buf + temp_len, str_buf, ret_len);
}

static void mtdoops_add_pmsg_head(char *oops_buf, enum mtdoops_log_type type)
{
	char str_buf[80] = {0};
	struct timespec64 now;
	struct tm ts;
	unsigned long local_time;
	int ret_len;

	ktime_get_coarse_real_ts64(&now);
	local_time = (unsigned long)(now.tv_sec + 8 * 60 * 60);
	time64_to_tm(local_time, 0, &ts);

	ret_len = snprintf(str_buf, sizeof(str_buf),
			   "\n```\n#### LOG TYPE:%s\n#####%04ld-%02d-%02d %02d:%02d:%02d\n```\n",
			   type < ARRAY_SIZE(log_type) ? log_type[type] : "Unknown",
			   ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
			   ts.tm_hour, ts.tm_min, ts.tm_sec);

	memcpy(oops_buf, str_buf, ret_len);
}

/*
 * Map the ramoops pmsg window. Mirrors the ram_vmap() body in
 * fs/pstore/ram_core.c so the window is reachable from the dumper.
 */
static void *mtdoops_ram_vmap(phys_addr_t start, size_t size)
{
	struct page **pages;
	phys_addr_t page_start;
	unsigned int page_count, i;
	void *vaddr;

	page_start = start - offset_in_page(start);
	page_count = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);

	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_err("%s: Failed to allocate array for %u pages\n",
		       __func__, page_count);
		return NULL;
	}

	for (i = 0; i < page_count; i++) {
		phys_addr_t addr = page_start + i * PAGE_SIZE;

		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}
	/*
	 * VM_IOREMAP bypasses this region during vread() and kmap_atomic()
	 * (i.e. kcore) to avoid __va() failures.
	 */
	vaddr = vmap(pages, page_count, VM_MAP | VM_IOREMAP, PAGE_KERNEL);
	kfree(pages);
	if (!vaddr)
		return NULL;

	return vaddr + offset_in_page(start);
}

/*
 * Copy the last logcat (pmsg) ring behind the kmsg text. The ramoops window
 * is laid out as console_size then pmsg_size; only the pmsg part is read.
 *
 * The window is a ring: @start is the next write offset and @size counts the
 * bytes currently stored, so the oldest byte sits at start - size (modulo the
 * ring length).
 */
static void mtdoops_ram_vunmap(void *addr)
{
	vunmap((void *)((unsigned long)addr & PAGE_MASK));
}

/*
 * Copy the last logcat (pmsg) ring behind the kmsg text. The ramoops window
 * is laid out as console_size then pmsg_size; only the pmsg part is read.
 *
 * The window is a ring: @start is the next write offset and @size counts the
 * bytes currently stored, so the oldest byte sits at start - size (modulo the
 * ring length).
 */
static size_t mtdoops_copy_pmsg(struct mtdoops_context *cxt, char *dst,
				size_t room)
{
	struct pmsg_buffer_hdr *hdr;
	phys_addr_t pstart;
	unsigned long ring, start, oldest;
	size_t marker, cp, first, second;
	void *va;

	if (!cxt->pmsg_data.pmsg_size || !cxt->pmsg_data.mem_address)
		return 0;

	pstart = cxt->pmsg_data.mem_address + cxt->pmsg_data.console_size;
	va = mtdoops_ram_vmap(pstart, cxt->pmsg_data.pmsg_size);
	if (!va)
		return 0;

	hdr = va;
	if (hdr->sig != MTDOOPS_PMSG_SIG) {
		pr_err("mtdoops: read pmsg failed sig = 0x%x\n", hdr->sig);
		mtdoops_ram_vunmap(va);
		return 0;
	}

	ring = cxt->pmsg_data.pmsg_size;
	marker = min_t(size_t, MTDOOPS_PMSG_HEAD_MAX, room);
	cp = min3((size_t)atomic_read(&hdr->size), (size_t)ring, room - marker);
	if (!cp) {
		mtdoops_ram_vunmap(va);
		return 0;
	}

	start = (unsigned long)atomic_read(&hdr->start);
	if (start >= ring)
		start %= ring;

	/*
	 * @start is the next write offset, so the stored range begins at
	 * start - cp (wrapped). Copy it forward from there.
	 */
	oldest = (start + ring - cp) % ring;
	first = min_t(size_t, cp, ring - oldest);
	second = cp - first;

	memcpy(dst + marker, hdr->data + oldest, first);
	if (second)
		memcpy(dst + marker + first, hdr->data, second);

	mtdoops_add_pmsg_head(dst, MTDOOPS_TYPE_PMSG);

	mtdoops_ram_vunmap(va);
	return cp + marker;
}

static void mtdoops_do_dump(struct mtdoops_context *cxt,
			    enum mtd_dump_reason reason)
{
	struct kmsg_dump_iter iter;
	size_t ret_len = 0;
	static int do_dump_count;
	int j, ret;

	if (!cxt->mtd)
		return;

	if (reason == MTD_DUMP_SHUTDOWN || reason == MTD_DUMP_EMERG)
		return;

	/* Only dump oopses if dump_oops is set */
	if (reason == MTD_DUMP_OOPS && !dump_oops)
		return;

	do_dump_count++;
	pr_err("%s start, count = %d, page = %d, reason = %d, dump_count = %d\n",
	       __func__, cxt->nextcount, cxt->nextpage, reason, do_dump_count);

	if (do_dump_count > 1) {
		for (j = 0, ret = -1; (j < 3) && (ret < 0); j++)
			ret = mtdoops_erase_block(cxt, cxt->nextpage * record_size);
	}

	kmsg_dump_rewind(&iter);

	if (test_and_set_bit(0, &cxt->oops_buf_busy))
		return;
	kmsg_dump_get_buffer(&iter, true,
			     cxt->oops_buf + MTDOOPS_HEADER_SIZE + MTDOOPS_BANNER_MAX,
			     lkmsg_record_size - MTDOOPS_HEADER_SIZE - MTDOOPS_BANNER_MAX,
			     &ret_len);
	clear_bit(0, &cxt->oops_buf_busy);

	/*
	 * Record layout: header, reason banner, kmsg tail, logcat tail.
	 * The banner is written last into the slot reserved above it.
	 */
	if (ret_len > record_size - MTDOOPS_HEADER_SIZE - MTDOOPS_BANNER_MAX)
		ret_len = record_size - MTDOOPS_HEADER_SIZE - MTDOOPS_BANNER_MAX;

	mtdoops_add_reason(cxt->oops_buf + MTDOOPS_HEADER_SIZE, reason,
			   MTDOOPS_TYPE_DMESG, cxt->nextcount);

	if (ret_len + MTDOOPS_HEADER_SIZE + MTDOOPS_BANNER_MAX < record_size) {
		size_t room = record_size - MTDOOPS_HEADER_SIZE -
			      MTDOOPS_BANNER_MAX - ret_len;

		mtdoops_copy_pmsg(cxt, cxt->oops_buf + MTDOOPS_HEADER_SIZE +
				  MTDOOPS_BANNER_MAX + ret_len, room);
	}

	if (reason == MTD_DUMP_OOPS || reason == MTD_DUMP_PANIC) {
		/* Panics must be written immediately */
		mtdoops_write(cxt, 1);
	} else {
		/*
		 * Write immediately: a scheduled work would lose the race
		 * against storage shutdown on restart/poweroff.
		 */
		mtdoops_write(cxt, 0);
	}

	pr_err("mtdoops_do_dump() finish\n");
}

static void mtdoops_kmsg_dump(struct kmsg_dumper *dumper,
			      struct kmsg_dump_detail *detail)
{
	struct mtdoops_context *cxt = container_of(dumper,
			struct mtdoops_context, dump);
	enum mtd_dump_reason reason;

	switch (detail->reason) {
	case KMSG_DUMP_PANIC:
		reason = MTD_DUMP_PANIC;
		break;
	case KMSG_DUMP_OOPS:
		reason = MTD_DUMP_OOPS;
		break;
	case KMSG_DUMP_EMERG:
		reason = MTD_DUMP_EMERG;
		break;
	case KMSG_DUMP_SHUTDOWN:
		reason = MTD_DUMP_SHUTDOWN;
		break;
	default:
		reason = MTD_DUMP_UNDEF;
		break;
	}

	mtdoops_do_dump(cxt, reason);
}

static int mtdoops_reboot_nb_handle(struct notifier_block *this,
				    unsigned long event, void *ptr)
{
	struct mtdoops_context *cxt = &oops_cxt;

	if (event == SYS_RESTART)
		mtdoops_do_dump(cxt, MTD_DUMP_RESTART);
	else if (event == SYS_POWER_OFF)
		mtdoops_do_dump(cxt, MTD_DUMP_POWEROFF);

	return NOTIFY_OK;
}

static void mtdoops_notify_add(struct mtd_info *mtd)
{
	struct mtdoops_context *cxt = &oops_cxt;
	u64 mtdoops_pages = div_u64(mtd->size, record_size);
	int err;

	if (!strcmp(mtd->name, mtddev))
		cxt->mtd_index = mtd->index;

	if (mtd->index != cxt->mtd_index || cxt->mtd_index < 0)
		return;

	if (mtd->size < mtd->erasesize * 2) {
		pr_err("MTD partition %d not big enough for mtdoops\n",
		       mtd->index);
		return;
	}
	if (mtd->erasesize < record_size) {
		pr_err("eraseblock size of MTD partition %d too small\n",
		       mtd->index);
		return;
	}
	if (mtd->size > MTDOOPS_MAX_MTD_SIZE) {
		pr_err("mtd%d is too large (limit is %d MiB)\n",
		       mtd->index, MTDOOPS_MAX_MTD_SIZE / 1024 / 1024);
		return;
	}

	/* oops_page_used is a bit field */
	cxt->oops_page_used =
		vmalloc_array(DIV_ROUND_UP(mtdoops_pages, BITS_PER_LONG),
			      sizeof(unsigned long));
	if (!cxt->oops_page_used) {
		pr_err("could not allocate page array\n");
		return;
	}

	cxt->dump.max_reason = KMSG_DUMP_MAX;
	cxt->dump.dump = mtdoops_kmsg_dump;
	err = kmsg_dump_register(&cxt->dump);
	if (err) {
		pr_err("registering kmsg dumper failed, error %d\n", err);
		vfree(cxt->oops_page_used);
		cxt->oops_page_used = NULL;
		return;
	}

	/* restart and poweroff */
	cxt->reboot_nb.notifier_call = mtdoops_reboot_nb_handle;
	cxt->reboot_nb.priority = 255;
	register_reboot_notifier(&cxt->reboot_nb);

	cxt->mtd = mtd;
	cxt->oops_pages = (int)mtd->size / record_size;
	find_next_position(cxt);
	pr_info("Attached to MTD device %d\n", mtd->index);
}

static void mtdoops_notify_remove(struct mtd_info *mtd)
{
	struct mtdoops_context *cxt = &oops_cxt;

	if (mtd->index != cxt->mtd_index || cxt->mtd_index < 0)
		return;

	if (kmsg_dump_unregister(&cxt->dump) < 0)
		pr_warn("could not unregister kmsg_dumper\n");

	unregister_reboot_notifier(&cxt->reboot_nb);

	cxt->mtd = NULL;
	flush_work(&cxt->work_erase);
}


static struct mtd_notifier mtdoops_notifier = {
	.add	= mtdoops_notify_add,
	.remove	= mtdoops_notify_remove,
};

static int mtdoops_parse_dt_u32(struct platform_device *pdev,
				const char *propname, u32 default_value,
				u32 *value)
{
	u32 val32 = 0;
	int ret;

	ret = of_property_read_u32(pdev->dev.of_node, propname, &val32);
	if (ret == -EINVAL) {
		/* field is missing, use default value */
		val32 = default_value;
	} else if (ret < 0) {
		pr_err("failed to parse property %s: %d\n", propname, ret);
		return ret;
	}
	if (val32 > INT_MAX) {
		pr_err("%s %u > INT_MAX\n", propname, val32);
		return -EOVERFLOW;
	}
	*value = val32;
	return 0;
}

static int mtdoops_pmsg_probe(struct platform_device *pdev)
{
	struct mtdoops_context *cxt = &oops_cxt;
	struct resource *res;
	u32 value;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("failed to locate DT /reserved-memory resource\n");
		return -EINVAL;
	}
	cxt->pmsg_data.mem_size = resource_size(res);
	cxt->pmsg_data.mem_address = res->start;

#define parse_u32(name, field, default_value) {				\
	ret = mtdoops_parse_dt_u32(pdev, name, default_value, &value);	\
	if (ret < 0)							\
		return ret;						\
	field = value;							\
}

	parse_u32("console-size", cxt->pmsg_data.console_size, 0);
	parse_u32("pmsg-size", cxt->pmsg_data.pmsg_size, 0);

#undef parse_u32

	pr_err("parse mtd_dt, mem_address = 0x%llx, mem_size = 0x%lx\n",
	       cxt->pmsg_data.mem_address, cxt->pmsg_data.mem_size);
	pr_err("parse mtd_dt, pmsg_size = 0x%lx, console-size = 0x%lx\n",
	       cxt->pmsg_data.pmsg_size, cxt->pmsg_data.console_size);

	return 0;
}

static const struct of_device_id mtdoops_pmsg_dt_match[] = {
	{ .compatible = "xiaomi,mtdoops_pmsg" },
	/* the boot monitor node carries the same console/pmsg window layout */
	{ .compatible = "xiaomi,bootmonitor_pmsg" },
	{}
};

static struct platform_driver mtdoops_pmsg_driver = {
	.probe	= mtdoops_pmsg_probe,
	.driver	= {
		.name		= "mtdoops_pmsg",
		.of_match_table	= mtdoops_pmsg_dt_match,
	},
};

static int __init mtdoops_init(void)
{
	struct mtdoops_context *cxt = &oops_cxt;
	int mtd_index;
	char *endp;

	if (strlen(mtddev) == 0) {
		pr_err("mtd device (mtddev=name/number) must be supplied\n");
		return -EINVAL;
	}
	if ((record_size & 4095) != 0) {
		pr_err("record_size must be a multiple of 4096\n");
		return -EINVAL;
	}
	if (record_size < 4096) {
		pr_err("record_size must be over 4096 bytes\n");
		return -EINVAL;
	}

	/* Setup the MTD device to use */
	cxt->mtd_index = -1;
	mtd_index = simple_strtoul(mtddev, &endp, 0);
	if (*endp == '\0')
		cxt->mtd_index = mtd_index;

	cxt->oops_buf = kmalloc(lkmsg_record_size, GFP_KERNEL);
	if (!cxt->oops_buf)
		return -ENOMEM;
	memset(cxt->oops_buf, 0xff, lkmsg_record_size);
	cxt->oops_buf_busy = 0;

	INIT_WORK(&cxt->work_erase, mtdoops_workfunc_erase);

	platform_driver_register(&mtdoops_pmsg_driver);
	register_mtd_user(&mtdoops_notifier);
	return 0;
}

static void __exit mtdoops_exit(void)
{
	struct mtdoops_context *cxt = &oops_cxt;

	unregister_mtd_user(&mtdoops_notifier);
	platform_driver_unregister(&mtdoops_pmsg_driver);
	kfree(cxt->oops_buf);
	vfree(cxt->oops_page_used);
}


vseq_module_init(mtdoops_init);
module_exit(mtdoops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Richard Purdie <rpurdie@openedhand.com>");
MODULE_DESCRIPTION("MTD Oops/Panic console logger/driver");
