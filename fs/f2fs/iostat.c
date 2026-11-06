// SPDX-License-Identifier: GPL-2.0
/*
 * f2fs iostat support
 *
 * Copyright 2021 Google LLC
 * Author: Daeho Jeong <daehojeong@google.com>
 */

#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include <linux/seq_file.h>

#include "f2fs.h"
#include "iostat.h"
#include <trace/events/f2fs.h>
#include <trace/hooks/fs.h>
#ifdef CONFIG_XIAOMI_ENHANCED_IOSTAT
#include <linux/sysfs.h>
#include <linux/ktime.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/bits.h>
#endif

static struct kmem_cache *bio_iostat_ctx_cache;
static mempool_t *bio_iostat_ctx_pool;

/*
 * Xiaomi enhanced IO latency statistics (XIAOMI_ENHANCED_IOSTAT).
 *
 * Standalone side statistics running next to the standard f2fs iostat
 * above: every f2fs bio submit/end pair is timed with ktime_get() (ns
 * resolution) and aggregated per-cpu into five size buckets (16K/128K/
 * 256K/512K/512+ KiB) split by read/write direction, into cumulative
 * tables, threshold-exceed tables (per size-bucket threshold, default
 * 100 ms each) and a 1/5/10/50 ms latency histogram.  The cumulative
 * tables roll up into double-buffered windows which are rotated every
 * iostat_window_period_s (default 60 s); the completed window stays
 * readable while the new one fills.  Objects are keyed by the backing
 * block device (sbi->sb->s_bdev->bd_dev) and live for the mount life
 * cycle only.  The standard iostat accounting, tracepoints and sysfs
 * nodes stay untouched: with the feature disabled through
 * en_iostat_enable the only per-IO work added here is one device lookup
 * and a flag test, and ctx->submit_ts keeps its stock jiffies
 * semantics.  (rodin 6.6 backport of the 6.6.118 feature)
 */

#define MI_IOSTAT_NR_SIZE_BUCKET	5
#define MI_IOSTAT_NR_LAT_BUCKET		5
#define MI_IOSTAT_NR_OBJ		5

/*
 * Size bucket edges: a bio lands in the bucket indexed by the number of
 * edges it exceeds, i.e. 16K/128K KiB on bi_size >> 10 and 256K/512K on
 * bi_size itself (the 1023 slack keeps exact-boundary bios in the lower
 * bucket).
 */
#define MI_IOSTAT_SIZE_KB_SMALL		16
#define MI_IOSTAT_SIZE_KB_MID		128
#define MI_IOSTAT_SIZE_BYTES_256K	0x403FF	/* 256K + 1023 */
#define MI_IOSTAT_SIZE_BYTES_512K	0x803FF	/* 512K + 1023 */

/* Latency histogram edges (ns): <1/<5/<10/<50/50+ ms. */
#define MI_IOSTAT_LAT_1MS	1000000ULL
#define MI_IOSTAT_LAT_5MS	5000000ULL
#define MI_IOSTAT_LAT_10MS	10000000ULL
#define MI_IOSTAT_LAT_50MS	50000000ULL

#define MI_IOSTAT_PERIOD_DEFAULT_NS	60000000000ULL	/* 60s */
#define MI_IOSTAT_THRESHOLD_DEFAULT_NS	100000000ULL	/* 100ms */

/*
 * Bit 63 marks a ktime-based submit anchor left by
 * mi_iostat_submit_anchor(); plain jiffies values from
 * iostat_update_submit_ctx() never carry it.
 */
#define MI_IOSTAT_TS_FLAG	BIT_ULL(63)

struct mi_iostat_entry {
	u64 sum_ns;
	u64 peak_ns;
	u32 cnt;
};

/* Per-cpu aggregation area, all counts in nanoseconds. */
struct mi_iostat_pcpu {
	u32 cur_idx;		/* window slot the writer currently fills */
	u64 rotate_ts;		/* ktime ns of the last window rotation */
	/* double buffered: [window slot][rw][size bucket] */
	struct mi_iostat_entry win_all[2][2][MI_IOSTAT_NR_SIZE_BUCKET];
	struct mi_iostat_entry win_exceed[2][2][MI_IOSTAT_NR_SIZE_BUCKET];
	/* cumulative since reset: [rw][size bucket] */
	struct mi_iostat_entry cum_all[2][MI_IOSTAT_NR_SIZE_BUCKET];
	struct mi_iostat_entry cum_exceed[2][MI_IOSTAT_NR_SIZE_BUCKET];
	/* cumulative latency histogram: [rw][latency bucket] */
	struct mi_iostat_entry hist[2][MI_IOSTAT_NR_LAT_BUCKET];
};

struct mi_iostat_obj {
	dev_t key;		/* sbi->sb->s_bdev->bd_dev */
	bool enable;		/* en_iostat_enable, off by default */
	u64 period_ns;		/* iostat_window_period_s, default 60s */
	u64 thresholds_ns[MI_IOSTAT_NR_SIZE_BUCKET];	/* per size bucket */
	struct mi_iostat_pcpu __percpu *pcpu;
};

static struct mi_iostat_obj *iostat_array[MI_IOSTAT_NR_OBJ];
static DEFINE_SPINLOCK(iostat_array_lock);

static const char *size_to_str[MI_IOSTAT_NR_SIZE_BUCKET] = {
	"16", "128", "256", "512", "512+"
};
static const char *io_type_to_str[2] = {
	"READ", "WRITE"
};
static const char *latency_range_to_str[MI_IOSTAT_NR_LAT_BUCKET] = {
	"1", "5", "10", "50", "50+"
};

static struct mi_iostat_obj *mi_iostat_obj_lookup(dev_t key)
{
	int i;

	for (i = 0; i < MI_IOSTAT_NR_OBJ; i++) {
		struct mi_iostat_obj *obj = iostat_array[i];

		if (obj && obj->key == key)
			return obj;
	}
	return NULL;
}

void *f2fs_get_iostat(struct f2fs_sb_info *sbi)
{
	if (!sbi || !sbi->sb || !sbi->sb->s_bdev)
		return NULL;
	return mi_iostat_obj_lookup(sbi->sb->s_bdev->bd_dev);
}

bool mi_iostat_get_enable(void *iostat)
{
	return ((struct mi_iostat_obj *)iostat)->enable;
}

void mi_iostat_set_enable(void *iostat, bool on)
{
	((struct mi_iostat_obj *)iostat)->enable = on;
}

u64 mi_iostat_get_period_ns(void *iostat)
{
	return ((struct mi_iostat_obj *)iostat)->period_ns;
}

void mi_iostat_set_period_ns(void *iostat, u64 ns)
{
	((struct mi_iostat_obj *)iostat)->period_ns = ns;
}

void mi_iostat_get_thresholds_ns(void *iostat, u64 ns[MI_IOSTAT_NR_SIZE_BUCKET])
{
	memcpy(ns, ((struct mi_iostat_obj *)iostat)->thresholds_ns,
			sizeof(((struct mi_iostat_obj *)iostat)->thresholds_ns));
}

void mi_iostat_set_thresholds_ns(void *iostat,
		const u64 ns[MI_IOSTAT_NR_SIZE_BUCKET])
{
	memcpy(((struct mi_iostat_obj *)iostat)->thresholds_ns, ns,
			sizeof(((struct mi_iostat_obj *)iostat)->thresholds_ns));
}

static unsigned int mi_iostat_size_bucket(struct bio *bio)
{
	unsigned int kb = bio->bi_iter.bi_size >> 10;
	unsigned int bucket = 0;

	if (kb > MI_IOSTAT_SIZE_KB_SMALL)
		bucket++;
	if (kb > MI_IOSTAT_SIZE_KB_MID)
		bucket++;
	if (bio->bi_iter.bi_size > MI_IOSTAT_SIZE_BYTES_256K)
		bucket++;
	if (bio->bi_iter.bi_size > MI_IOSTAT_SIZE_BYTES_512K)
		bucket++;
	return bucket;
}

static unsigned int mi_iostat_latency_bucket(u64 lat_ns)
{
	unsigned int bucket = 0;

	if (lat_ns >= MI_IOSTAT_LAT_1MS)
		bucket++;
	if (lat_ns >= MI_IOSTAT_LAT_5MS)
		bucket++;
	if (lat_ns >= MI_IOSTAT_LAT_10MS)
		bucket++;
	if (lat_ns >= MI_IOSTAT_LAT_50MS)
		bucket++;
	return bucket;
}

static void mi_iostat_entry_update(struct mi_iostat_entry *e, u64 lat_ns)
{
	e->sum_ns += lat_ns;
	if (e->peak_ns < lat_ns)
		e->peak_ns = lat_ns;
	e->cnt++;
}

static void mi_iostat_account(struct mi_iostat_obj *obj, struct bio *bio,
		u64 now, u64 lat_ns)
{
	struct mi_iostat_pcpu *pcpu;
	unsigned int rw = op_is_write(bio_op(bio));
	unsigned int size = mi_iostat_size_bucket(bio);
	unsigned int latb = mi_iostat_latency_bucket(lat_ns);
	struct mi_iostat_entry *e;
	unsigned int idx;

	preempt_disable();
	pcpu = per_cpu_ptr(obj->pcpu, smp_processor_id());
	idx = pcpu->cur_idx;

	if (now - pcpu->rotate_ts > obj->period_ns) {
		unsigned int new_idx = (~idx) & 1;

		pcpu->cur_idx = new_idx;
		pcpu->rotate_ts = now;
		memset(pcpu->win_all[new_idx], 0,
				sizeof(pcpu->win_all[new_idx]));
		memset(pcpu->win_exceed[new_idx], 0,
				sizeof(pcpu->win_exceed[new_idx]));
		idx = new_idx;
	}

	mi_iostat_entry_update(&pcpu->win_all[idx][rw][size], lat_ns);
	mi_iostat_entry_update(&pcpu->cum_all[rw][size], lat_ns);

	if (lat_ns > obj->thresholds_ns[size]) {
		e = &pcpu->win_exceed[idx][rw][size];
		e->sum_ns += lat_ns;
		e->peak_ns = pcpu->win_all[idx][rw][size].peak_ns;
		e->cnt++;

		e = &pcpu->cum_exceed[rw][size];
		e->sum_ns += lat_ns;
		e->peak_ns = pcpu->cum_all[rw][size].peak_ns;
		e->cnt++;
	}

	mi_iostat_entry_update(&pcpu->hist[rw][latb], lat_ns);
	preempt_enable();
}

/*
 * Enhanced accounting on the completion path.  Runs before the standard
 * iostat code: it restores the stock jiffies semantics of
 * ctx->submit_ts from the ktime anchor, so the standard latency
 * accounting below sees exactly the values it would see without this
 * feature, then folds the nanosecond latency into the enhanced tables.
 */
static void mi_iostat_update(struct bio_iostat_ctx *iostat_ctx,
		struct bio *bio)
{
	u64 anchored = iostat_ctx->submit_ts;
	struct mi_iostat_obj *obj;
	u64 now, lat_ns;

	if (!(anchored & MI_IOSTAT_TS_FLAG))
		return;
	anchored &= ~MI_IOSTAT_TS_FLAG;

	now = ktime_get();
	lat_ns = now - anchored;
	iostat_ctx->submit_ts = jiffies - nsecs_to_jiffies64(lat_ns);

	obj = mi_iostat_obj_lookup(iostat_ctx->sbi->sb->s_bdev->bd_dev);
	if (!obj || !obj->enable)
		return;

	mi_iostat_account(obj, bio, now, lat_ns);
}

/*
 * Submit-side anchor: stamped right before submit_bio() through the
 * android_vh_f2fs_iostat_submit hook in f2fs_submit_read_bio()/
 * f2fs_submit_write_bio().  Only samples of mounts whose enhanced
 * statistics are enabled get the ktime anchor; everyone else keeps the
 * plain jiffies value written by iostat_update_submit_ctx().
 */
static void mi_iostat_submit_anchor(void *unused, struct super_block *sb,
		int type, struct bio *bio)
{
	struct bio_iostat_ctx *iostat_ctx = bio->bi_private;
	struct mi_iostat_obj *obj;

	if (!iostat_ctx || !sb || !sb->s_bdev)
		return;

	obj = mi_iostat_obj_lookup(sb->s_bdev->bd_dev);
	if (!obj || !obj->enable)
		return;

	iostat_ctx->submit_ts = ktime_get() | MI_IOSTAT_TS_FLAG;
}

ssize_t iostat_daily_latency_show(struct f2fs_sb_info *sbi, char *buf)
{
	struct mi_iostat_obj *obj = f2fs_get_iostat(sbi);
	ssize_t len = 0;
	unsigned int rw, size, b;
	int cpu;

	if (!obj || !obj->enable)
		return sysfs_emit(buf, "I/O stats not available\n");

	len += sysfs_emit_at(buf, len, "Normal:\n%-10s %-16s %-11s %-16s\n",
			"Size(k)", "Sum(ms)", "Count", "Peak(ms)");
	for (rw = 0; rw < 2; rw++) {
		len += sysfs_emit_at(buf, len, "[%s]\n", rw ? "WRITE" : "READ");
		for (size = 0; size < MI_IOSTAT_NR_SIZE_BUCKET; size++) {
			u64 sum = 0, peak = 0;
			u32 cnt = 0;

			for_each_possible_cpu(cpu) {
				struct mi_iostat_pcpu *pc =
						per_cpu_ptr(obj->pcpu, cpu);
				struct mi_iostat_entry *e =
						&pc->cum_all[rw][size];

				sum += e->sum_ns;
				if (peak < e->peak_ns)
					peak = e->peak_ns;
				cnt += e->cnt;
			}
			len += sysfs_emit_at(buf, len,
					"%-10s %-16llu %-11u %-16llu\n",
					size_to_str[size],
					(u64)(sum / 1000000ULL), cnt,
					(u64)(peak / 1000000ULL));
		}
	}

	len += sysfs_emit_at(buf, len,
			"\nAbnormal:\n%-10s %-16s %-11s %-16s %-16s\n",
			"Size(k)", "Sum(ms)", "Count", "Peak(ms)",
			"Threshold(ms)");
	for (rw = 0; rw < 2; rw++) {
		len += sysfs_emit_at(buf, len, "[%s]\n", rw ? "WRITE" : "READ");
		for (size = 0; size < MI_IOSTAT_NR_SIZE_BUCKET; size++) {
			u64 sum = 0, peak = 0;
			u32 cnt = 0;

			for_each_possible_cpu(cpu) {
				struct mi_iostat_pcpu *pc =
						per_cpu_ptr(obj->pcpu, cpu);
				struct mi_iostat_entry *e =
						&pc->cum_exceed[rw][size];

				sum += e->sum_ns;
				if (peak < e->peak_ns)
					peak = e->peak_ns;
				cnt += e->cnt;
			}
			len += sysfs_emit_at(buf, len,
					"%-10s %-16llu %-11u %-16llu %-16lu\n",
					size_to_str[size],
					(u64)(sum / 1000000ULL), cnt,
					(u64)(peak / 1000000ULL),
					(unsigned long)
					(obj->thresholds_ns[size] / 1000000ULL));
		}
	}

	len += sysfs_emit_at(buf, len, "\nDelay:\n%-10s %-16s %-11s %-16s\n",
			"Range(ms)", "Sum(ms)", "Count", "Peak(ms)");
	for (rw = 0; rw < 2; rw++) {
		len += sysfs_emit_at(buf, len, "[%s]\n", rw ? "WRITE" : "READ");
		for (b = 0; b < MI_IOSTAT_NR_LAT_BUCKET; b++) {
			u64 sum = 0, peak = 0;
			u32 cnt = 0;

			for_each_possible_cpu(cpu) {
				struct mi_iostat_pcpu *pc =
						per_cpu_ptr(obj->pcpu, cpu);
				struct mi_iostat_entry *e = &pc->hist[rw][b];

				sum += e->sum_ns;
				if (peak < e->peak_ns)
					peak = e->peak_ns;
				cnt += e->cnt;
			}
			len += sysfs_emit_at(buf, len,
					"%-10s %-16llu %-11u %-16llu\n",
					latency_range_to_str[b],
					(u64)(sum / 1000000ULL), cnt,
					(u64)(peak / 1000000ULL));
		}
	}

	return len;
}

ssize_t iostat_window_latency_show(struct f2fs_sb_info *sbi, char *buf)
{
	struct mi_iostat_obj *obj = f2fs_get_iostat(sbi);
	u64 now, period, bound_cur, bound_prev;
	ssize_t len = 0;
	unsigned int rw, size;
	int cpu;

	if (!obj || !obj->enable)
		return sysfs_emit(buf, "I/O stats not available\n");

	now = ktime_get();
	period = obj->period_ns;
	bound_cur = period * 2;		/* window being filled */
	bound_prev = period * 3;	/* completed window */

	len += sysfs_emit_at(buf, len, "Normal:\n%-8s %-12s %-16s %-16s %-16s\n",
			"Window", "Size(k)", "Sum(ms)", "Count", "Peak(ms)");
	for (rw = 0; rw < 2; rw++) {
		len += sysfs_emit_at(buf, len, "[%s]\n", io_type_to_str[rw]);
		for (size = 0; size < MI_IOSTAT_NR_SIZE_BUCKET; size++) {
			u64 sum = 0, peak = 0;
			u32 cnt = 0, win = 0;

			for_each_possible_cpu(cpu) {
				struct mi_iostat_pcpu *pc =
						per_cpu_ptr(obj->pcpu, cpu);
				struct mi_iostat_entry *e;
				unsigned int idx;

				if (now - pc->rotate_ts >
						(rw ? bound_prev : bound_cur))
					continue;
				idx = rw ? ((~pc->cur_idx) & 1) :
						(pc->cur_idx & 1);
				win = idx;
				e = &pc->win_all[idx][rw][size];
				sum += e->sum_ns;
				if (peak < e->peak_ns)
					peak = e->peak_ns;
				cnt += e->cnt;
			}
			if (!cnt)
				continue;
			len += sysfs_emit_at(buf, len,
					"%-8d %-12s %-16llu %-16u %-16llu\n",
					(int)win, size_to_str[size],
					(u64)(sum / 1000000ULL), cnt,
					(u64)(peak / 1000000ULL));
		}
	}

	len += sysfs_emit_at(buf, len,
			"\nAbnormal:\n%-8s %-12s %-16s %-16s %-16s %-16s\n",
			"Window", "Size(k)", "Sum(ms)", "Count", "Peak(ms)",
			"Threshold(ms)");
	for (rw = 0; rw < 2; rw++) {
		len += sysfs_emit_at(buf, len, "[%s]\n", io_type_to_str[rw]);
		for (size = 0; size < MI_IOSTAT_NR_SIZE_BUCKET; size++) {
			u64 sum = 0, peak = 0;
			u32 cnt = 0, win = 0;

			for_each_possible_cpu(cpu) {
				struct mi_iostat_pcpu *pc =
						per_cpu_ptr(obj->pcpu, cpu);
				struct mi_iostat_entry *e;
				unsigned int idx;

				if (now - pc->rotate_ts >
						(rw ? bound_prev : bound_cur))
					continue;
				idx = rw ? ((~pc->cur_idx) & 1) :
						(pc->cur_idx & 1);
				win = idx;
				e = &pc->win_exceed[idx][rw][size];
				sum += e->sum_ns;
				if (peak < e->peak_ns)
					peak = e->peak_ns;
				cnt += e->cnt;
			}
			if (!cnt)
				continue;
			len += sysfs_emit_at(buf, len,
					"%-8d %-12s %-16llu %-16u %-16llu %-16lu\n",
					(int)win, size_to_str[size],
					(u64)(sum / 1000000ULL), cnt,
					(u64)(peak / 1000000ULL),
					(unsigned long)
					(obj->thresholds_ns[size] / 1000000ULL));
		}
	}

	return len;
}

/*
 * Store side of iostat_daily_latency / iostat_window_latency: writing
 * "reset" clears the cumulative tables (the double buffered windows
 * keep rolling on their own schedule); anything else is rejected.
 */
ssize_t iostat_daily_stats_reset(struct f2fs_sb_info *sbi, const char *buf,
		size_t len)
{
	struct mi_iostat_obj *obj = f2fs_get_iostat(sbi);
	char *p;
	int cpu;

	if (!obj || !obj->enable)
		return -ENODEV;

	p = strim((char *)buf);
	if (strncmp(p, "reset", 5))
		return -EINVAL;

	for_each_possible_cpu(cpu) {
		struct mi_iostat_pcpu *pc = per_cpu_ptr(obj->pcpu, cpu);

		memset(pc->cum_all, 0, sizeof(pc->cum_all));
		memset(pc->cum_exceed, 0, sizeof(pc->cum_exceed));
		memset(pc->hist, 0, sizeof(pc->hist));
	}
	return len;
}

/* Full reset of every enhanced table, used when the feature is disabled. */
void iostat_latency_stats_reset(struct f2fs_sb_info *sbi)
{
	struct mi_iostat_obj *obj = f2fs_get_iostat(sbi);
	int cpu;

	if (!obj)
		return;

	for_each_possible_cpu(cpu) {
		struct mi_iostat_pcpu *pc = per_cpu_ptr(obj->pcpu, cpu);

		memset(pc->win_all, 0, sizeof(pc->win_all));
		memset(pc->win_exceed, 0, sizeof(pc->win_exceed));
		memset(pc->cum_all, 0, sizeof(pc->cum_all));
		memset(pc->cum_exceed, 0, sizeof(pc->cum_exceed));
		memset(pc->hist, 0, sizeof(pc->hist));
	}
}

static int mi_iostat_obj_create(struct f2fs_sb_info *sbi)
{
	struct mi_iostat_obj *obj;
	dev_t key = sbi->sb->s_bdev->bd_dev;
	int i, cpu;

	spin_lock(&iostat_array_lock);
	obj = mi_iostat_obj_lookup(key);
	spin_unlock(&iostat_array_lock);
	if (obj)
		return 0;	/* shared with an already registered mount */

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return -ENOMEM;

	obj->key = key;
	obj->enable = false;
	obj->period_ns = MI_IOSTAT_PERIOD_DEFAULT_NS;
	for (i = 0; i < MI_IOSTAT_NR_SIZE_BUCKET; i++)
		obj->thresholds_ns[i] = MI_IOSTAT_THRESHOLD_DEFAULT_NS;

	obj->pcpu = alloc_percpu(struct mi_iostat_pcpu);
	if (!obj->pcpu) {
		kfree(obj);
		return -ENOMEM;
	}

	for_each_possible_cpu(cpu) {
		struct mi_iostat_pcpu *pc = per_cpu_ptr(obj->pcpu, cpu);

		memset(pc, 0, sizeof(*pc));
		pc->rotate_ts = ktime_get();
	}

	spin_lock(&iostat_array_lock);
	if (mi_iostat_obj_lookup(key)) {
		/* lost a race against a same-device mount */
		spin_unlock(&iostat_array_lock);
		free_percpu(obj->pcpu);
		kfree(obj);
		return 0;
	}
	for (i = 0; i < MI_IOSTAT_NR_OBJ; i++) {
		if (!iostat_array[i]) {
			iostat_array[i] = obj;
			break;
		}
	}
	spin_unlock(&iostat_array_lock);

	if (i == MI_IOSTAT_NR_OBJ) {
		/* table full: this mount runs without enhanced statistics */
		free_percpu(obj->pcpu);
		kfree(obj);
	}
	return 0;
}

static void mi_iostat_obj_destroy(struct f2fs_sb_info *sbi)
{
	dev_t key = sbi->sb->s_bdev->bd_dev;
	int i;

	spin_lock(&iostat_array_lock);
	for (i = 0; i < MI_IOSTAT_NR_OBJ; i++) {
		struct mi_iostat_obj *obj = iostat_array[i];

		if (obj && obj->key == key) {
			iostat_array[i] = NULL;
			spin_unlock(&iostat_array_lock);
			free_percpu(obj->pcpu);
			kfree(obj);
			return;
		}
	}
	spin_unlock(&iostat_array_lock);
}

static inline unsigned long long iostat_get_avg_bytes(struct f2fs_sb_info *sbi,
	enum iostat_type type)
{
	return sbi->iostat_count[type] ? div64_u64(sbi->iostat_bytes[type],
		sbi->iostat_count[type]) : 0;
}

#define IOSTAT_INFO_SHOW(name, type)					\
	seq_printf(seq, "%-23s %-16llu %-16llu %-16llu\n",		\
			name":", sbi->iostat_bytes[type],		\
			sbi->iostat_count[type],			\
			iostat_get_avg_bytes(sbi, type))

int __maybe_unused iostat_info_seq_show(struct seq_file *seq, void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);

	if (!sbi->iostat_enable)
		return 0;

	seq_printf(seq, "time:		%-16llu\n", ktime_get_real_seconds());
	seq_printf(seq, "\t\t\t%-16s %-16s %-16s\n",
				"io_bytes", "count", "avg_bytes");

	/* print app write IOs */
	seq_puts(seq, "[WRITE]\n");
	IOSTAT_INFO_SHOW("app buffered data", APP_BUFFERED_IO);
	IOSTAT_INFO_SHOW("app direct data", APP_DIRECT_IO);
	IOSTAT_INFO_SHOW("app mapped data", APP_MAPPED_IO);
	IOSTAT_INFO_SHOW("app buffered cdata", APP_BUFFERED_CDATA_IO);
	IOSTAT_INFO_SHOW("app mapped cdata", APP_MAPPED_CDATA_IO);

	/* print fs write IOs */
	IOSTAT_INFO_SHOW("fs data", FS_DATA_IO);
	IOSTAT_INFO_SHOW("fs cdata", FS_CDATA_IO);
	IOSTAT_INFO_SHOW("fs node", FS_NODE_IO);
	IOSTAT_INFO_SHOW("fs meta", FS_META_IO);
	IOSTAT_INFO_SHOW("fs gc data", FS_GC_DATA_IO);
	IOSTAT_INFO_SHOW("fs gc node", FS_GC_NODE_IO);
	IOSTAT_INFO_SHOW("fs cp data", FS_CP_DATA_IO);
	IOSTAT_INFO_SHOW("fs cp node", FS_CP_NODE_IO);
	IOSTAT_INFO_SHOW("fs cp meta", FS_CP_META_IO);

	/* print app read IOs */
	seq_puts(seq, "[READ]\n");
	IOSTAT_INFO_SHOW("app buffered data", APP_BUFFERED_READ_IO);
	IOSTAT_INFO_SHOW("app direct data", APP_DIRECT_READ_IO);
	IOSTAT_INFO_SHOW("app mapped data", APP_MAPPED_READ_IO);
	IOSTAT_INFO_SHOW("app buffered cdata", APP_BUFFERED_CDATA_READ_IO);
	IOSTAT_INFO_SHOW("app mapped cdata", APP_MAPPED_CDATA_READ_IO);

	/* print fs read IOs */
	IOSTAT_INFO_SHOW("fs data", FS_DATA_READ_IO);
	IOSTAT_INFO_SHOW("fs gc data", FS_GDATA_READ_IO);
	IOSTAT_INFO_SHOW("fs cdata", FS_CDATA_READ_IO);
	IOSTAT_INFO_SHOW("fs node", FS_NODE_READ_IO);
	IOSTAT_INFO_SHOW("fs meta", FS_META_READ_IO);

	/* print other IOs */
	seq_puts(seq, "[OTHER]\n");
	IOSTAT_INFO_SHOW("fs discard", FS_DISCARD_IO);
	IOSTAT_INFO_SHOW("fs flush", FS_FLUSH_IO);
	IOSTAT_INFO_SHOW("fs zone reset", FS_ZONE_RESET_IO);

	return 0;
}

static inline void __record_iostat_latency(struct f2fs_sb_info *sbi)
{
	int io, idx;
	struct f2fs_iostat_latency iostat_lat[MAX_IO_TYPE][NR_PAGE_TYPE];
	struct iostat_lat_info *io_lat = sbi->iostat_io_lat;
	unsigned long flags;

	spin_lock_irqsave(&sbi->iostat_lat_lock, flags);
	for (idx = 0; idx < MAX_IO_TYPE; idx++) {
		for (io = 0; io < NR_PAGE_TYPE; io++) {
			iostat_lat[idx][io].peak_lat =
			   jiffies_to_msecs(io_lat->peak_lat[idx][io]);
			iostat_lat[idx][io].cnt = io_lat->bio_cnt[idx][io];
			iostat_lat[idx][io].avg_lat = iostat_lat[idx][io].cnt ?
			   jiffies_to_msecs(io_lat->sum_lat[idx][io]) / iostat_lat[idx][io].cnt : 0;
			io_lat->sum_lat[idx][io] = 0;
			io_lat->peak_lat[idx][io] = 0;
			io_lat->bio_cnt[idx][io] = 0;
		}
	}
	spin_unlock_irqrestore(&sbi->iostat_lat_lock, flags);

	trace_f2fs_iostat_latency(sbi, iostat_lat);
}

static inline void f2fs_record_iostat(struct f2fs_sb_info *sbi)
{
	unsigned long long iostat_diff[NR_IO_TYPE];
	int i;
	unsigned long flags;

	if (time_is_after_jiffies(sbi->iostat_next_period))
		return;

	/* Need double check under the lock */
	spin_lock_irqsave(&sbi->iostat_lock, flags);
	if (time_is_after_jiffies(sbi->iostat_next_period)) {
		spin_unlock_irqrestore(&sbi->iostat_lock, flags);
		return;
	}
	sbi->iostat_next_period = jiffies +
				msecs_to_jiffies(sbi->iostat_period_ms);

	for (i = 0; i < NR_IO_TYPE; i++) {
		iostat_diff[i] = sbi->iostat_bytes[i] -
				sbi->prev_iostat_bytes[i];
		sbi->prev_iostat_bytes[i] = sbi->iostat_bytes[i];
	}
	spin_unlock_irqrestore(&sbi->iostat_lock, flags);

	trace_f2fs_iostat(sbi, iostat_diff);

	__record_iostat_latency(sbi);
}

void f2fs_reset_iostat(struct f2fs_sb_info *sbi)
{
	struct iostat_lat_info *io_lat = sbi->iostat_io_lat;
	int i;

	spin_lock_irq(&sbi->iostat_lock);
	for (i = 0; i < NR_IO_TYPE; i++) {
		sbi->iostat_count[i] = 0;
		sbi->iostat_bytes[i] = 0;
		sbi->prev_iostat_bytes[i] = 0;
	}
	spin_unlock_irq(&sbi->iostat_lock);

	spin_lock_irq(&sbi->iostat_lat_lock);
	memset(io_lat, 0, sizeof(struct iostat_lat_info));
	spin_unlock_irq(&sbi->iostat_lat_lock);
}

static inline void __f2fs_update_iostat(struct f2fs_sb_info *sbi,
			enum iostat_type type, unsigned long long io_bytes)
{
	sbi->iostat_bytes[type] += io_bytes;
	sbi->iostat_count[type]++;
}

void f2fs_update_iostat(struct f2fs_sb_info *sbi, struct inode *inode,
			enum iostat_type type, unsigned long long io_bytes)
{
	unsigned long flags;

	if (!sbi->iostat_enable)
		return;

	spin_lock_irqsave(&sbi->iostat_lock, flags);
	__f2fs_update_iostat(sbi, type, io_bytes);

	if (type == APP_BUFFERED_IO || type == APP_DIRECT_IO)
		__f2fs_update_iostat(sbi, APP_WRITE_IO, io_bytes);

	if (type == APP_BUFFERED_READ_IO || type == APP_DIRECT_READ_IO)
		__f2fs_update_iostat(sbi, APP_READ_IO, io_bytes);

#ifdef CONFIG_F2FS_FS_COMPRESSION
	if (inode && f2fs_compressed_file(inode)) {
		if (type == APP_BUFFERED_IO)
			__f2fs_update_iostat(sbi, APP_BUFFERED_CDATA_IO, io_bytes);

		if (type == APP_BUFFERED_READ_IO)
			__f2fs_update_iostat(sbi, APP_BUFFERED_CDATA_READ_IO, io_bytes);

		if (type == APP_MAPPED_READ_IO)
			__f2fs_update_iostat(sbi, APP_MAPPED_CDATA_READ_IO, io_bytes);

		if (type == APP_MAPPED_IO)
			__f2fs_update_iostat(sbi, APP_MAPPED_CDATA_IO, io_bytes);

		if (type == FS_DATA_READ_IO)
			__f2fs_update_iostat(sbi, FS_CDATA_READ_IO, io_bytes);

		if (type == FS_DATA_IO)
			__f2fs_update_iostat(sbi, FS_CDATA_IO, io_bytes);
	}
#endif

	spin_unlock_irqrestore(&sbi->iostat_lock, flags);

	f2fs_record_iostat(sbi);
}

static inline void __update_iostat_latency(struct bio_iostat_ctx *iostat_ctx,
				enum iostat_lat_type lat_type)
{
	unsigned long ts_diff;
	unsigned int page_type = iostat_ctx->type;
	struct f2fs_sb_info *sbi = iostat_ctx->sbi;
	struct iostat_lat_info *io_lat = sbi->iostat_io_lat;
	unsigned long flags;

	if (!sbi->iostat_enable)
		return;

	ts_diff = jiffies - iostat_ctx->submit_ts;
	if (page_type == META_FLUSH) {
		page_type = META;
	} else if (page_type >= NR_PAGE_TYPE) {
		f2fs_warn(sbi, "%s: %d over NR_PAGE_TYPE", __func__, page_type);
		return;
	}

	spin_lock_irqsave(&sbi->iostat_lat_lock, flags);
	io_lat->sum_lat[lat_type][page_type] += ts_diff;
	io_lat->bio_cnt[lat_type][page_type]++;
	if (ts_diff > io_lat->peak_lat[lat_type][page_type])
		io_lat->peak_lat[lat_type][page_type] = ts_diff;
	spin_unlock_irqrestore(&sbi->iostat_lat_lock, flags);
}

void iostat_update_and_unbind_ctx(struct bio *bio)
{
	struct bio_iostat_ctx *iostat_ctx = bio->bi_private;
	enum iostat_lat_type lat_type;
	bool skip = false;

#ifdef CONFIG_XIAOMI_ENHANCED_IOSTAT
	mi_iostat_update(iostat_ctx, bio);
#endif

	trace_android_vh_f2fs_iostat_update(iostat_ctx->sbi->sb, bio, &skip);
	if (op_is_write(bio_op(bio))) {
		lat_type = bio->bi_opf & REQ_SYNC ?
				WRITE_SYNC_IO : WRITE_ASYNC_IO;
		bio->bi_private = iostat_ctx->sbi;
	} else {
		lat_type = READ_IO;
		bio->bi_private = iostat_ctx->post_read_ctx;
	}

	if (!skip)
		__update_iostat_latency(iostat_ctx, lat_type);
	mempool_free(iostat_ctx, bio_iostat_ctx_pool);
}

void iostat_alloc_and_bind_ctx(struct f2fs_sb_info *sbi,
		struct bio *bio, struct bio_post_read_ctx *ctx)
{
	struct bio_iostat_ctx *iostat_ctx;
	/* Due to the mempool, this never fails. */
	iostat_ctx = mempool_alloc(bio_iostat_ctx_pool, GFP_NOFS);
	iostat_ctx->sbi = sbi;
	iostat_ctx->submit_ts = 0;
	iostat_ctx->type = 0;
	iostat_ctx->post_read_ctx = ctx;
	bio->bi_private = iostat_ctx;
}

int __init f2fs_init_iostat_processing(void)
{
	bio_iostat_ctx_cache =
		kmem_cache_create("f2fs_bio_iostat_ctx",
				  sizeof(struct bio_iostat_ctx), 0, 0, NULL);
	if (!bio_iostat_ctx_cache)
		goto fail;
	bio_iostat_ctx_pool =
		mempool_create_slab_pool(NUM_PREALLOC_IOSTAT_CTXS,
					 bio_iostat_ctx_cache);
	if (!bio_iostat_ctx_pool)
		goto fail_free_cache;
#ifdef CONFIG_XIAOMI_ENHANCED_IOSTAT
	register_trace_android_vh_f2fs_iostat_submit(mi_iostat_submit_anchor,
			NULL);
#endif
	return 0;

fail_free_cache:
	kmem_cache_destroy(bio_iostat_ctx_cache);
fail:
	return -ENOMEM;
}

void f2fs_destroy_iostat_processing(void)
{
#ifdef CONFIG_XIAOMI_ENHANCED_IOSTAT
	unregister_trace_android_vh_f2fs_iostat_submit(mi_iostat_submit_anchor,
			NULL);
	tracepoint_synchronize_unregister();
#endif
	mempool_destroy(bio_iostat_ctx_pool);
	kmem_cache_destroy(bio_iostat_ctx_cache);
}

int f2fs_init_iostat(struct f2fs_sb_info *sbi)
{
	/* init iostat info */
	spin_lock_init(&sbi->iostat_lock);
	spin_lock_init(&sbi->iostat_lat_lock);
	sbi->iostat_enable = false;
	sbi->iostat_period_ms = DEFAULT_IOSTAT_PERIOD_MS;
	sbi->iostat_io_lat = f2fs_kzalloc(sbi, sizeof(struct iostat_lat_info),
					GFP_KERNEL);
	if (!sbi->iostat_io_lat)
		return -ENOMEM;

#ifdef CONFIG_XIAOMI_ENHANCED_IOSTAT
	return mi_iostat_obj_create(sbi);
#else
	return 0;
#endif
}

void f2fs_destroy_iostat(struct f2fs_sb_info *sbi)
{
#ifdef CONFIG_XIAOMI_ENHANCED_IOSTAT
	mi_iostat_obj_destroy(sbi);
#endif
	kfree(sbi->iostat_io_lat);
}
