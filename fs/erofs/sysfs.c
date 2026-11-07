// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C), 2008-2021, OPPO Mobile Comm Corp., Ltd.
 *             https://www.oppo.com/
 */
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/ktime.h>

#include "internal.h"
#include "compress.h"

enum {
	attr_feature,
	attr_drop_caches,
	attr_pointer_ui,
	attr_pointer_bool,
	attr_accel,
	attr_iostat_config,
	attr_iostat_window_latency,
	attr_iostat_daily_latency,
};

enum {
	struct_erofs_sb_info,
	struct_erofs_mount_opts,
	struct_erofs_iostat_obj,
};

struct erofs_attr {
	struct attribute attr;
	short attr_id;
	int struct_type, offset;
};

#define EROFS_ATTR(_name, _mode, _id)					\
static struct erofs_attr erofs_attr_##_name = {				\
	.attr = {.name = __stringify(_name), .mode = _mode },		\
	.attr_id = attr_##_id,						\
}
#define EROFS_ATTR_FUNC(_name, _mode)	EROFS_ATTR(_name, _mode, _name)
#define EROFS_ATTR_FEATURE(_name)	EROFS_ATTR(_name, 0444, feature)

#define EROFS_ATTR_OFFSET(_name, _mode, _id, _struct)	\
static struct erofs_attr erofs_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.attr_id = attr_##_id,					\
	.struct_type = struct_##_struct,			\
	.offset = offsetof(struct _struct, _name),\
}

#define EROFS_ATTR_RW(_name, _id, _struct)	\
	EROFS_ATTR_OFFSET(_name, 0644, _id, _struct)

#define EROFS_RO_ATTR(_name, _id, _struct)	\
	EROFS_ATTR_OFFSET(_name, 0444, _id, _struct)

#define EROFS_ATTR_RW_UI(_name, _struct)	\
	EROFS_ATTR_RW(_name, pointer_ui, _struct)

#define EROFS_ATTR_RW_BOOL(_name, _struct)	\
	EROFS_ATTR_RW(_name, pointer_bool, _struct)

#define ATTR_LIST(name) (&erofs_attr_##name.attr)

#ifdef CONFIG_EROFS_FS_ZIP
EROFS_ATTR_RW_UI(sync_decompress, erofs_sb_info);
EROFS_ATTR_FUNC(drop_caches, 0200);
#endif
#ifdef CONFIG_EROFS_FS_ZIP_ACCEL
EROFS_ATTR_FUNC(accel, 0644);
#endif
EROFS_ATTR_RW_UI(dir_ra_bytes, erofs_sb_info);

#ifdef CONFIG_XIAOMI_EROFS_IOSTAT
/* iostat_enable rides the generic bool case over the iostat object */
static struct erofs_attr erofs_attr_iostat_enable = {
	.attr = {.name = __stringify(iostat_enable), .mode = 0644 },
	.attr_id = attr_pointer_bool,
	.struct_type = struct_erofs_iostat_obj,
	.offset = offsetof(struct erofs_iostat_obj, enable),
};
static struct erofs_attr erofs_attr_iostat_config = {
	.attr = {.name = __stringify(iostat_config), .mode = 0644 },
	.attr_id = attr_iostat_config,
	.struct_type = struct_erofs_iostat_obj,
	.offset = 0,
};
EROFS_ATTR_FUNC(iostat_window_latency, 0444);
EROFS_ATTR_FUNC(iostat_daily_latency, 0644);
#endif

static struct attribute *erofs_sb_attrs[] = {
#ifdef CONFIG_EROFS_FS_ZIP
	ATTR_LIST(sync_decompress),
	ATTR_LIST(drop_caches),
#endif
	ATTR_LIST(dir_ra_bytes),
#ifdef CONFIG_XIAOMI_EROFS_IOSTAT
	ATTR_LIST(iostat_enable),
	ATTR_LIST(iostat_config),
	ATTR_LIST(iostat_window_latency),
	ATTR_LIST(iostat_daily_latency),
#endif
	NULL,
};
ATTRIBUTE_GROUPS(erofs_sb);

static struct attribute *erofs_attrs[] = {
#ifdef CONFIG_EROFS_FS_ZIP_ACCEL
	ATTR_LIST(accel),
#endif
	NULL,
};
ATTRIBUTE_GROUPS(erofs);

/* Features this copy of erofs supports */
EROFS_ATTR_FEATURE(zero_padding);
EROFS_ATTR_FEATURE(compr_cfgs);
EROFS_ATTR_FEATURE(big_pcluster);
EROFS_ATTR_FEATURE(chunked_file);
EROFS_ATTR_FEATURE(device_table);
EROFS_ATTR_FEATURE(compr_head2);
EROFS_ATTR_FEATURE(sb_chksum);
EROFS_ATTR_FEATURE(ztailpacking);
EROFS_ATTR_FEATURE(fragments);
EROFS_ATTR_FEATURE(dedupe);
EROFS_ATTR_FEATURE(48bit);
EROFS_ATTR_FEATURE(metabox);

static struct attribute *erofs_feat_attrs[] = {
	ATTR_LIST(zero_padding),
	ATTR_LIST(compr_cfgs),
	ATTR_LIST(big_pcluster),
	ATTR_LIST(chunked_file),
	ATTR_LIST(device_table),
	ATTR_LIST(compr_head2),
	ATTR_LIST(sb_chksum),
	ATTR_LIST(ztailpacking),
	ATTR_LIST(fragments),
	ATTR_LIST(dedupe),
	ATTR_LIST(48bit),
	ATTR_LIST(metabox),
	NULL,
};
ATTRIBUTE_GROUPS(erofs_feat);

static unsigned char *__struct_ptr(struct erofs_sb_info *sbi,
					  int struct_type, int offset)
{
	if (struct_type == struct_erofs_sb_info)
		return (unsigned char *)sbi + offset;
	if (struct_type == struct_erofs_mount_opts)
		return (unsigned char *)&sbi->opt + offset;
#ifdef CONFIG_XIAOMI_EROFS_IOSTAT
	if (struct_type == struct_erofs_iostat_obj)
		return (unsigned char *)sbi->iostat + offset;
#endif
	return NULL;
}

static ssize_t erofs_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct erofs_sb_info *sbi = container_of(kobj, struct erofs_sb_info,
						s_kobj);
	struct erofs_attr *a = container_of(attr, struct erofs_attr, attr);
	unsigned char *ptr = __struct_ptr(sbi, a->struct_type, a->offset);

	switch (a->attr_id) {
	case attr_feature:
		return sysfs_emit(buf, "supported\n");
	case attr_pointer_ui:
		if (!ptr)
			return 0;
		return sysfs_emit(buf, "%u\n", *(unsigned int *)ptr);
	case attr_pointer_bool:
		if (!ptr)
			return 0;
		return sysfs_emit(buf, "%d\n", *(bool *)ptr);
#ifdef CONFIG_XIAOMI_EROFS_IOSTAT
	case attr_iostat_config: {
		struct erofs_iostat_obj *obj = sbi->iostat;

		if (!obj)
			return 0;
		return sysfs_emit(buf, "%lu,%lu,%lu,%lu,%lu,%lu\n",
			(unsigned long)(obj->period_ns / NSEC_PER_SEC),
			(unsigned long)(obj->thresholds_ns[0] / NSEC_PER_MSEC),
			(unsigned long)(obj->thresholds_ns[1] / NSEC_PER_MSEC),
			(unsigned long)(obj->thresholds_ns[2] / NSEC_PER_MSEC),
			(unsigned long)(obj->thresholds_ns[3] / NSEC_PER_MSEC),
			(unsigned long)(obj->thresholds_ns[4] / NSEC_PER_MSEC));
	}
	case attr_iostat_window_latency:
		if (!ptr)
			return 0;
		return erofs_iostat_window_latency_show(sbi, buf);
	case attr_iostat_daily_latency:
		if (!ptr)
			return 0;
		return erofs_iostat_daily_latency_show(sbi, buf);
#endif
	case attr_accel:
		return z_erofs_crypto_show_engines(buf, PAGE_SIZE, '\n');
	}
	return 0;
}

static ssize_t erofs_attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t len)
{
	struct erofs_sb_info *sbi = container_of(kobj, struct erofs_sb_info,
						s_kobj);
	struct erofs_attr *a = container_of(attr, struct erofs_attr, attr);
	unsigned char *ptr = __struct_ptr(sbi, a->struct_type, a->offset);
	unsigned long t;
	int ret;

	switch (a->attr_id) {
	case attr_pointer_ui:
		if (!ptr)
			return 0;
		ret = kstrtoul(skip_spaces(buf), 0, &t);
		if (ret)
			return ret;
		if (t != (unsigned int)t)
			return -ERANGE;
#ifdef CONFIG_EROFS_FS_ZIP
		if (!strcmp(a->attr.name, "sync_decompress") &&
		    (t > EROFS_SYNC_DECOMPRESS_FORCE_OFF))
			return -EINVAL;
#endif
		*(unsigned int *)ptr = t;
		return len;
	case attr_pointer_bool:
		if (!ptr)
			return 0;
		ret = kstrtoul(skip_spaces(buf), 0, &t);
		if (ret)
			return ret;
		if (t != 0 && t != 1)
			return -EINVAL;
		*(bool *)ptr = !!t;
#ifdef CONFIG_XIAOMI_EROFS_IOSTAT
		if (!strcmp(a->attr.name, "iostat_enable")) {
			struct erofs_iostat_obj *obj = sbi->iostat;

			if (obj && !obj->enable)
				erofs_iostat_latency_stats_reset(sbi);
		}
#endif
		return len;
#ifdef CONFIG_XIAOMI_EROFS_IOSTAT
	case attr_iostat_config:
		if (!ptr)
			return 0;
		ret = erofs_iostat_config_parse(sbi, buf);
		return ret ? ret : len;
	case attr_iostat_daily_latency:
		if (!ptr)
			return 0;
		ret = erofs_iostat_daily_stats_reset(sbi, buf);
		return ret ? ret : len;
#endif
#ifdef CONFIG_EROFS_FS_ZIP
	case attr_drop_caches:
		ret = kstrtoul(skip_spaces(buf), 0, &t);
		if (ret)
			return ret;
		if (t < 1 || t > 3)
			return -EINVAL;

		if (t & 2)
			z_erofs_shrink_scan(sbi, ~0UL);
		if (t & 1)
			invalidate_mapping_pages(MNGD_MAPPING(sbi), 0, -1);
		return len;
#endif
#ifdef CONFIG_EROFS_FS_ZIP_ACCEL
	case attr_accel:
		buf = skip_spaces(buf);
		z_erofs_crypto_disable_all_engines();
		while (*buf) {
			t = strcspn(buf, "\n");
			ret = z_erofs_crypto_enable_engine(buf, t);
			if (ret < 0)
				return ret;
			buf += buf[t] != '\0' ? t + 1 : t;
		}
		return len;
#endif
	}
	return 0;
}

static void erofs_sb_release(struct kobject *kobj)
{
	struct erofs_sb_info *sbi = container_of(kobj, struct erofs_sb_info,
						 s_kobj);
	complete(&sbi->s_kobj_unregister);
}

static const struct sysfs_ops erofs_attr_ops = {
	.show	= erofs_attr_show,
	.store	= erofs_attr_store,
};

static const struct kobj_type erofs_sb_ktype = {
	.default_groups = erofs_sb_groups,
	.sysfs_ops	= &erofs_attr_ops,
	.release	= erofs_sb_release,
};

static const struct kobj_type erofs_ktype = {
	.default_groups = erofs_groups,
	.sysfs_ops	= &erofs_attr_ops,
};

static struct kset erofs_root = {
	.kobj	= {.ktype = &erofs_ktype},
};

static const struct kobj_type erofs_feat_ktype = {
	.default_groups = erofs_feat_groups,
	.sysfs_ops	= &erofs_attr_ops,
};

static struct kobject erofs_feat = {
	.kset	= &erofs_root,
};

int erofs_register_sysfs(struct super_block *sb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	int err;

	sbi->s_kobj.kset = &erofs_root;
	init_completion(&sbi->s_kobj_unregister);
	err = kobject_init_and_add(&sbi->s_kobj, &erofs_sb_ktype, NULL, "%s",
				   sb->s_sysfs_name);
	if (err) {
		kobject_put(&sbi->s_kobj);
		wait_for_completion(&sbi->s_kobj_unregister);
	}
	return err;
}

void erofs_unregister_sysfs(struct super_block *sb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);

	if (sbi->s_kobj.state_in_sysfs) {
		kobject_del(&sbi->s_kobj);
		kobject_put(&sbi->s_kobj);
		wait_for_completion(&sbi->s_kobj_unregister);
	}
}

void erofs_exit_sysfs(void)
{
	kobject_put(&erofs_feat);
	kset_unregister(&erofs_root);
}

int __init erofs_init_sysfs(void)
{
	int ret;

	kobject_set_name(&erofs_root.kobj, "erofs");
	erofs_root.kobj.parent = fs_kobj;
	ret = kset_register(&erofs_root);
	if (!ret) {
		ret = kobject_init_and_add(&erofs_feat, &erofs_feat_ktype,
					   NULL, "features");
		if (!ret)
			return 0;
		erofs_exit_sysfs();
	}
	return ret;
}
