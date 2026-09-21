// SPDX-License-Identifier: GPL-2.0
#include <linux/blkdev.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/slab.h>

#ifdef CONFIG_MODULE_FORCE_LOAD

struct cp_bdev_open {
	struct list_head list;
	struct block_device *bdev;
	struct file *bdev_file;
};

static LIST_HEAD(cp_bdev_opens);
static DEFINE_MUTEX(cp_bdev_lock);

struct block_device *blkdev_get_by_path(const char *path, blk_mode_t mode,
					void *holder,
					const struct blk_holder_ops *hops);
void blkdev_put(struct block_device *bdev, void *holder);

struct block_device *blkdev_get_by_path(const char *path, blk_mode_t mode,
					void *holder,
					const struct blk_holder_ops *hops)
{
	struct cp_bdev_open *e;
	struct file *file;

	file = bdev_file_open_by_path(path, mode, holder, hops);
	if (IS_ERR(file))
		return ERR_CAST(file);

	e = kmalloc(sizeof(*e), GFP_KERNEL);
	if (!e) {
		bdev_fput(file);
		return ERR_PTR(-ENOMEM);
	}

	e->bdev_file = file;
	e->bdev = file_bdev(file);

	mutex_lock(&cp_bdev_lock);
	list_add(&e->list, &cp_bdev_opens);
	mutex_unlock(&cp_bdev_lock);

	return e->bdev;
}
EXPORT_SYMBOL(blkdev_get_by_path);

void blkdev_put(struct block_device *bdev, void *holder)
{
	struct cp_bdev_open *e, *found = NULL;
	struct file *file = NULL;

	mutex_lock(&cp_bdev_lock);
	list_for_each_entry(e, &cp_bdev_opens, list) {
		if (e->bdev == bdev) {
			found = e;
			file = e->bdev_file;
			list_del(&e->list);
			break;
		}
	}
	mutex_unlock(&cp_bdev_lock);

	if (!found) {
		pr_warn("blkdev_put: no 6.6-compat open for that device\n");
		return;
	}

	kfree(found);
	bdev_fput(file);
}
EXPORT_SYMBOL(blkdev_put);

#endif
