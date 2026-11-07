// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 Xiaomi Corporation.
 *
 * erofs read-path IO latency statistics.  Latency samples are taken per
 * read bio and aggregated per superblock into per-cpu size/latency
 * buckets, exported through the per-mount sysfs attributes
 * iostat_enable/iostat_config/iostat_daily_latency/iostat_window_latency.
 *
 * The samples ride the in-tree android_vh_erofs_iostat_submit/update
 * hooks: the submit side stamps bio->android_oem_data1 with ktime right
 * before submit_bio(), the update side folds the latency into the
 * per-cpu tables at endio and clears the slot.  Hooks are registered
 * while at least one mounted superblock carries an iostat object.
 */
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/preempt.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include <trace/hooks/fs.h>

#include "internal.h"

/* size buckets: 16K / 128K / 256K / 512K / 512K+ */
static const char * const size_str[5] = { "16", "128", "256", "512", "512+" };

/* latency buckets: <1ms / 1-5ms / 5-10ms / 10-50ms / 50ms+ */
static const char * const latency_range_str[5] = { "1", "5", "10", "50", "50+" };

/*
 * per-cpu statistics working area.  daily[] and exceed_rot[] are double
 * buffered and rotated every period_ns, the rest accumulate since enable.
 */
struct erofs_iostat_cpu {
	unsigned int idx;				/* current buffer slot */
	u64 rotate_ts;					/* last rotation timestamp */
	struct erofs_iostat_entry daily[2][5];		/* per-window size buckets */
	struct erofs_iostat_entry exceed_rot[2][5];	/* per-window over-threshold */
	struct erofs_iostat_entry window[5];		/* fixed size buckets */
	struct erofs_iostat_entry exceed_fix[5];	/* fixed over-threshold */
	struct erofs_iostat_entry hist[5];		/* latency histogram */
};

#define EROFS_IOSTAT_ACC(ent, sum, peak, cnt)	\
	do {					\
		(sum) += (ent)->sum;		\
		if ((peak) < (ent)->peak)	\
			(peak) = (ent)->peak;	\
		(cnt) += (ent)->cnt;		\
	} while (0)

/*
 * Hook registration state: the tracepoints are global, so the submit/update
 * consumers are registered once for the first iostat-capable mount and
 * dropped again with the last one.  Every callback early-returns through
 * sbi->iostat for mounts without (or with a disabled) statistics object.
 */
static DEFINE_MUTEX(erofs_iostat_hook_lock);
static unsigned int erofs_iostat_hook_users;

static int erofs_iostat_size_bucket(u32 size)
{
	u32 kb = size >> 10;
	int bucket = 0;

	if (kb > 16)
		bucket = 1;
	if (kb > 128)
		bucket = 2;
	if (size > 0x403ff)
		bucket = 3;
	if (size > 0x803ff)
		bucket = 4;
	return bucket;
}

static int erofs_iostat_latency_bucket(u64 lat)
{
	if (lat < NSEC_PER_MSEC)
		return 0;
	if (lat < 5 * NSEC_PER_MSEC)
		return 1;
	if (lat < 10 * NSEC_PER_MSEC)
		return 2;
	if (lat < 50 * NSEC_PER_MSEC)
		return 3;
	return 4;
}

static void erofs_iostat_record_start(struct erofs_sb_info *sbi, struct bio *bio)
{
	struct erofs_iostat_obj *obj = sbi->iostat;

	if (!obj || !obj->enable)
		return;

	/* idempotent: a bio must never restart its latency window */
	if (!bio->android_oem_data1)
		bio->android_oem_data1 = ktime_get();
}

static void erofs_iostat_update(struct erofs_sb_info *sbi, struct bio *bio)
{
	struct erofs_iostat_obj *obj = sbi->iostat;
	struct erofs_iostat_cpu *pc;
	struct erofs_iostat_entry *e;
	u64 ts = bio->android_oem_data1;
	u64 now, lat;
	unsigned int size, slot;
	int bucket, lat_bucket;

	if (!obj || !obj->enable)
		return;
	if (!ts)
		return;

	size = bio->bi_iter.bi_size;
	now = ktime_get();
	lat = now - ts;

	bucket = erofs_iostat_size_bucket(size);
	lat_bucket = erofs_iostat_latency_bucket(lat);

	preempt_disable();
	pc = this_cpu_ptr(obj->percpu);

	slot = pc->idx;
	if (now - pc->rotate_ts > obj->period_ns) {
		slot = ~slot & 1;
		pc->idx = slot;
		pc->rotate_ts = now;
		memset(pc->daily[slot], 0, sizeof(pc->daily[slot]));
		memset(pc->exceed_rot[slot], 0, sizeof(pc->exceed_rot[slot]));
	}

	e = &pc->daily[slot][bucket];
	e->sum += lat;
	if (e->peak < lat)
		e->peak = lat;
	e->cnt++;

	e = &pc->window[bucket];
	e->sum += lat;
	if (e->peak < lat)
		e->peak = lat;
	e->cnt++;

	if (lat > obj->thresholds_ns[bucket]) {
		e = &pc->exceed_rot[slot][bucket];
		e->sum += lat;
		e->peak = pc->daily[slot][bucket].peak;
		e->cnt++;

		e = &pc->exceed_fix[bucket];
		e->sum += lat;
		e->peak = pc->window[bucket].peak;
		e->cnt++;
	}

	e = &pc->hist[lat_bucket];
	e->sum += lat;
	if (e->peak < lat)
		e->peak = lat;
	e->cnt++;

	preempt_enable();

	bio->android_oem_data1 = 0;
}

static void erofs_iostat_submit_hook(void *unused, struct super_block *sb,
				     struct bio *bio)
{
	if (!sb)
		return;
	erofs_iostat_record_start(EROFS_SB(sb), bio);
}

static void erofs_iostat_update_hook(void *unused, struct super_block *sb,
				     struct bio *bio)
{
	if (!sb)
		return;
	erofs_iostat_update(EROFS_SB(sb), bio);
}

int erofs_iostat_config_parse(struct erofs_sb_info *sbi, const char *buf)
{
	struct erofs_iostat_obj *obj = sbi->iostat;
	u64 vals[6] = { };
	char *s, *token;
	int i, ret;

	if (!obj)
		return -EINVAL;

	s = strim((char *)buf);
	for (i = 0; i < 6; i++) {
		token = strsep(&s, ",");
		if (!token)
			return -EINVAL;
		ret = kstrtoull(token, 10, &vals[i]);
		if (ret)
			return -EINVAL;
	}
	/* exactly six comma separated fields are accepted */
	if (s)
		return -EINVAL;
	if (vals[0] > U32_MAX)
		return -EINVAL;

	obj->period_ns = vals[0] * NSEC_PER_SEC;
	for (i = 0; i < 5; i++)
		obj->thresholds_ns[i] = vals[i + 1] * NSEC_PER_MSEC;
	return 0;
}

int erofs_iostat_daily_stats_reset(struct erofs_sb_info *sbi, const char *buf)
{
	struct erofs_iostat_obj *obj = sbi->iostat;
	struct erofs_iostat_cpu *pc;
	int cpu;

	if (!obj || !obj->enable)
		return -ENODATA;
	if (strncmp(strim((char *)buf), "reset", 5))
		return -EINVAL;

	for_each_possible_cpu(cpu) {
		pc = per_cpu_ptr(obj->percpu, cpu);
		memset(pc->window, 0, sizeof(pc->window));
		memset(pc->exceed_fix, 0, sizeof(pc->exceed_fix));
		memset(pc->hist, 0, sizeof(pc->hist));
	}
	return 0;
}

void erofs_iostat_latency_stats_reset(struct erofs_sb_info *sbi)
{
	struct erofs_iostat_obj *obj = sbi->iostat;
	struct erofs_iostat_cpu *pc;
	int cpu;

	if (!obj)
		return;

	for_each_possible_cpu(cpu) {
		pc = per_cpu_ptr(obj->percpu, cpu);
		memset(pc->daily, 0, sizeof(pc->daily));
		memset(pc->exceed_rot, 0, sizeof(pc->exceed_rot));
		memset(pc->window, 0, sizeof(pc->window));
		memset(pc->exceed_fix, 0, sizeof(pc->exceed_fix));
		memset(pc->hist, 0, sizeof(pc->hist));
	}
}

int erofs_iostat_daily_latency_show(struct erofs_sb_info *sbi, char *buf)
{
	struct erofs_iostat_obj *obj = sbi->iostat;
	struct erofs_iostat_cpu *pc;
	struct erofs_iostat_entry *e;
	u64 sum, peak;
	u32 cnt;
	int i, cpu, len = 0;

	if (!obj || !obj->enable)
		return sysfs_emit(buf, "I/O stats not available\n");

	len += sysfs_emit_at(buf, len, "Normal:\n%-12s %-16s %-16s %-16s\n",
			     "Size(k)", "Sum(ms)", "Count", "Peak(ms)");
	for (i = 0; i < 5; i++) {
		sum = 0;
		peak = 0;
		cnt = 0;
		for_each_possible_cpu(cpu) {
			pc = per_cpu_ptr(obj->percpu, cpu);
			e = &pc->window[i];
			EROFS_IOSTAT_ACC(e, sum, peak, cnt);
		}
		len += sysfs_emit_at(buf, len,
				     "%-12s %-16llu %-16u %-16llu\n",
				     size_str[i], sum / NSEC_PER_MSEC, cnt,
				     peak / NSEC_PER_MSEC);
	}

	len += sysfs_emit_at(buf, len,
			     "\nAbnormal:\n%-12s %-16s %-16s %-16s %-16s\n",
			     "Size(k)", "Sum(ms)", "Count", "Peak(ms)",
			     "Threshold(ms)");
	for (i = 0; i < 5; i++) {
		sum = 0;
		peak = 0;
		cnt = 0;
		for_each_possible_cpu(cpu) {
			pc = per_cpu_ptr(obj->percpu, cpu);
			e = &pc->exceed_fix[i];
			EROFS_IOSTAT_ACC(e, sum, peak, cnt);
		}
		len += sysfs_emit_at(buf, len,
				     "%-12s %-16llu %-16u %-16llu %-16lu\n",
				     size_str[i], sum / NSEC_PER_MSEC, cnt,
				     peak / NSEC_PER_MSEC,
				     (unsigned long)
				     (obj->thresholds_ns[i] / NSEC_PER_MSEC));
	}

	len += sysfs_emit_at(buf, len, "\nDelay:\n%-10s %-16s %-11s %-16s\n",
			     "Range(ms)", "Sum(ms)", "Count", "Peak(ms)");
	for (i = 0; i < 5; i++) {
		sum = 0;
		peak = 0;
		cnt = 0;
		for_each_possible_cpu(cpu) {
			pc = per_cpu_ptr(obj->percpu, cpu);
			e = &pc->hist[i];
			EROFS_IOSTAT_ACC(e, sum, peak, cnt);
		}
		len += sysfs_emit_at(buf, len,
				     "%-10s %-16llu %-11u %-16llu\n",
				     latency_range_str[i], sum / NSEC_PER_MSEC,
				     cnt, peak / NSEC_PER_MSEC);
	}
	return len;
}

/*
 * aggregate one bucket over all cpus whose window is still fresh, taking
 * the current or the last completed buffer slot; reports the slot used so
 * the caller can print the window column.
 */
static void erofs_iostat_window_collect(struct erofs_iostat_obj *obj, int idx,
					bool exceed, bool prev, u64 now,
					u64 bound, u64 *sum, u64 *peak,
					u32 *cnt, u32 *slot)
{
	struct erofs_iostat_cpu *pc;
	struct erofs_iostat_entry *e;
	int cpu;

	*sum = 0;
	*peak = 0;
	*cnt = 0;
	*slot = 0;
	for_each_possible_cpu(cpu) {
		pc = per_cpu_ptr(obj->percpu, cpu);
		if (now - pc->rotate_ts > bound)
			continue;
		*slot = prev ? ~pc->idx & 1 : pc->idx & 1;
		e = exceed ? &pc->exceed_rot[*slot][idx] :
			     &pc->daily[*slot][idx];
		*sum += e->sum;
		if (*peak < e->peak)
			*peak = e->peak;
		*cnt += e->cnt;
	}
}

int erofs_iostat_window_latency_show(struct erofs_sb_info *sbi, char *buf)
{
	struct erofs_iostat_obj *obj = sbi->iostat;
	u64 now, sum, peak;
	u32 cnt, slot;
	int i, len = 0;

	if (!obj || !obj->enable)
		return sysfs_emit(buf, "I/O stats not available\n");

	now = ktime_get();

	len += sysfs_emit_at(buf, len,
			     "Normal:\n%-8s %-12s %-16s %-16s %-16s\n",
			     "Window", "Size(k)", "Sum(ms)", "Count",
			     "Peak(ms)");
	for (i = 0; i < 5; i++) {
		erofs_iostat_window_collect(obj, i, false, false, now,
					    obj->period_ns * 2, &sum, &peak,
					    &cnt, &slot);
		if (!cnt)
			continue;
		len += sysfs_emit_at(buf, len,
				     "%-8d %-12s %-16llu %-16u %-16llu\n",
				     slot, size_str[i], sum / NSEC_PER_MSEC,
				     cnt, peak / NSEC_PER_MSEC);
	}
	for (i = 0; i < 5; i++) {
		erofs_iostat_window_collect(obj, i, false, true, now,
					    obj->period_ns * 3, &sum, &peak,
					    &cnt, &slot);
		if (!cnt)
			continue;
		len += sysfs_emit_at(buf, len,
				     "%-8d %-12s %-16llu %-16u %-16llu\n",
				     slot, size_str[i], sum / NSEC_PER_MSEC,
				     cnt, peak / NSEC_PER_MSEC);
	}

	len += sysfs_emit_at(buf, len,
			     "\nAbnormal:\n%-8s %-12s %-16s %-16s %-16s %-16s\n",
			     "Window", "Size", "Sum(ms)", "Count", "Peak(ms)",
			     "Threshold(ms)");
	for (i = 0; i < 5; i++) {
		erofs_iostat_window_collect(obj, i, true, false, now,
					    obj->period_ns * 2, &sum, &peak,
					    &cnt, &slot);
		if (!cnt)
			continue;
		len += sysfs_emit_at(buf, len,
				     "%-8d %-12s %-16llu %-16u %-16llu %-16lu\n",
				     slot, size_str[i], sum / NSEC_PER_MSEC,
				     cnt, peak / NSEC_PER_MSEC,
				     (unsigned long)
				     (obj->thresholds_ns[i] / NSEC_PER_MSEC));
	}
	for (i = 0; i < 5; i++) {
		erofs_iostat_window_collect(obj, i, true, true, now,
					    obj->period_ns * 3, &sum, &peak,
					    &cnt, &slot);
		if (!cnt)
			continue;
		len += sysfs_emit_at(buf, len,
				     "%-8d %-12s %-16llu %-16u %-16llu %-16lu\n",
				     slot, size_str[i], sum / NSEC_PER_MSEC,
				     cnt, peak / NSEC_PER_MSEC,
				     (unsigned long)
				     (obj->thresholds_ns[i] / NSEC_PER_MSEC));
	}
	return len;
}

int erofs_init_iostat(struct erofs_sb_info *sbi)
{
	struct erofs_iostat_obj *obj;
	struct erofs_iostat_cpu *pc;
	int ret = 0;
	int i, cpu;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return -ENOMEM;

	obj->enable = 0;
	obj->period_ns = 60ULL * NSEC_PER_SEC;
	for (i = 0; i < 5; i++)
		obj->thresholds_ns[i] = 100ULL * NSEC_PER_MSEC;

	obj->percpu = __alloc_percpu(sizeof(struct erofs_iostat_cpu),
				     __alignof__(u64));
	if (!obj->percpu) {
		kfree(obj);
		return -ENOMEM;
	}

	for_each_possible_cpu(cpu) {
		pc = per_cpu_ptr(obj->percpu, cpu);
		memset(pc, 0, sizeof(*pc));
		pc->idx = 0;
		pc->rotate_ts = ktime_get();
	}

	mutex_lock(&erofs_iostat_hook_lock);
	if (erofs_iostat_hook_users++ == 0) {
		ret = register_trace_android_vh_erofs_iostat_submit(
				erofs_iostat_submit_hook, NULL);
		if (!ret)
			ret = register_trace_android_vh_erofs_iostat_update(
					erofs_iostat_update_hook, NULL);
		if (ret) {
			unregister_trace_android_vh_erofs_iostat_submit(
					erofs_iostat_submit_hook, NULL);
			unregister_trace_android_vh_erofs_iostat_update(
					erofs_iostat_update_hook, NULL);
			erofs_iostat_hook_users--;
			mutex_unlock(&erofs_iostat_hook_lock);
			free_percpu(obj->percpu);
			kfree(obj);
			return ret;
		}
	}
	mutex_unlock(&erofs_iostat_hook_lock);

	/* published last: callbacks observe a fully built object or NULL */
	sbi->iostat = obj;
	return 0;
}

void erofs_destroy_iostat(struct erofs_sb_info *sbi)
{
	struct erofs_iostat_obj *obj = sbi->iostat;

	if (!obj)
		return;
	/* unpublish first so in-flight callbacks early-return */
	sbi->iostat = NULL;

	mutex_lock(&erofs_iostat_hook_lock);
	if (--erofs_iostat_hook_users == 0) {
		unregister_trace_android_vh_erofs_iostat_submit(
				erofs_iostat_submit_hook, NULL);
		unregister_trace_android_vh_erofs_iostat_update(
				erofs_iostat_update_hook, NULL);
		tracepoint_synchronize_unregister();
	}
	mutex_unlock(&erofs_iostat_hook_lock);

	free_percpu(obj->percpu);
	kfree(obj);
}
