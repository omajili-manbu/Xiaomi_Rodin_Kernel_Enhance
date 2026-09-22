// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6-compat: block queue-limit helpers 6.18 inlined into blkdev.h,
 * plus removed bdev APIs, for prebuilt 6.6 modules (virtio_blk, zram,
 * ufs-mediatek-mod, mtd_blkdevs, mtk-mmc).
 *
 * The 6.18 static-inline bodies are renamed via macros, then re-exported
 * out-of-line under the 6.6 names.
 */
#define blk_queue_logical_block_size	blk_queue_logical_block_size_618
#define blk_queue_physical_block_size	blk_queue_physical_block_size_618
#define blk_queue_alignment_offset	blk_queue_alignment_offset_618
#define blk_queue_io_min		blk_queue_io_min_618
#define blk_queue_io_opt		blk_queue_io_opt_618
#define blk_queue_chunk_sectors		blk_queue_chunk_sectors_618
#define blk_queue_max_hw_sectors	blk_queue_max_hw_sectors_618
#define blk_queue_max_segments		blk_queue_max_segments_618
#define blk_queue_max_segment_size	blk_queue_max_segment_size_618
#define blk_queue_max_discard_sectors	blk_queue_max_discard_sectors_618
#define blk_queue_max_discard_segments	blk_queue_max_discard_segments_618
#define blk_queue_max_secure_erase_sectors blk_queue_max_secure_erase_sectors_618
#define blk_queue_max_write_zeroes_sectors blk_queue_max_write_zeroes_sectors_618
#define blk_queue_max_zone_append_sectors blk_queue_max_zone_append_sectors_618
#define blk_queue_write_cache		blk_queue_write_cache_618
#define blk_mq_freeze_queue		blk_mq_freeze_queue_618
#define blk_mq_unfreeze_queue		blk_mq_unfreeze_queue_618
#define disk_set_zoned			disk_set_zoned_618
#define sg_next				sg_next_618
#define mmc_can_gpio_cd			mmc_can_gpio_cd_618

#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/blk-integrity.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/scatterlist.h>
#include <linux/shrinker.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>

/* 6.18 turned these names into macros; the 6.6 symbols come back below */
#ifdef blk_queue_logical_block_size
#undef blk_queue_logical_block_size
#endif
#ifdef blk_queue_physical_block_size
#undef blk_queue_physical_block_size
#endif
#ifdef blk_queue_alignment_offset
#undef blk_queue_alignment_offset
#endif
#ifdef blk_queue_io_min
#undef blk_queue_io_min
#endif
#ifdef blk_queue_io_opt
#undef blk_queue_io_opt
#endif
#ifdef blk_queue_chunk_sectors
#undef blk_queue_chunk_sectors
#endif
#ifdef blk_queue_max_hw_sectors
#undef blk_queue_max_hw_sectors
#endif
#ifdef blk_queue_max_segments
#undef blk_queue_max_segments
#endif
#ifdef blk_queue_max_segment_size
#undef blk_queue_max_segment_size
#endif
#ifdef blk_queue_max_discard_sectors
#undef blk_queue_max_discard_sectors
#endif
#ifdef blk_queue_max_discard_segments
#undef blk_queue_max_discard_segments
#endif
#ifdef blk_queue_max_secure_erase_sectors
#undef blk_queue_max_secure_erase_sectors
#endif
#ifdef blk_queue_max_write_zeroes_sectors
#undef blk_queue_max_write_zeroes_sectors
#endif
#ifdef blk_queue_max_zone_append_sectors
#undef blk_queue_max_zone_append_sectors
#endif
#ifdef blk_queue_write_cache
#undef blk_queue_write_cache
#endif
#ifdef blk_mq_freeze_queue
#undef blk_mq_freeze_queue
#endif
#ifdef blk_mq_unfreeze_queue
#undef blk_mq_unfreeze_queue
#endif
#ifdef blk_mq_init_queue
#undef blk_mq_init_queue
#endif
#ifdef blk_queue_update_dma_pad
#undef blk_queue_update_dma_pad
#endif
#ifdef sg_next
#undef sg_next
#endif
#ifdef mmc_can_gpio_cd
#undef mmc_can_gpio_cd
#endif
#ifdef blk_mq_virtio_map_queues
#undef blk_mq_virtio_map_queues
#endif
#ifdef disk_set_zoned
#undef disk_set_zoned
#endif
#ifdef virtqueue_disable_dma_api_for_buffers
#undef virtqueue_disable_dma_api_for_buffers
#endif


#undef blk_queue_logical_block_size
#undef blk_queue_physical_block_size
#undef blk_queue_alignment_offset
#undef blk_queue_io_min
#undef blk_queue_io_opt
#undef blk_queue_chunk_sectors
#undef blk_queue_max_hw_sectors
#undef blk_queue_max_segments
#undef blk_queue_max_segment_size
#undef blk_queue_max_discard_sectors
#undef blk_queue_max_discard_segments
#undef blk_queue_max_secure_erase_sectors
#undef blk_queue_max_write_zeroes_sectors
#undef blk_queue_max_zone_append_sectors
#undef blk_queue_write_cache
#undef blk_mq_freeze_queue
#undef blk_mq_unfreeze_queue
#undef disk_set_zoned
#undef sg_next
#undef mmc_can_gpio_cd

/* 6.6-ABI prototypes: 6.18 inlined or deleted these names. */
void blk_queue_update_dma_pad(struct request_queue *q, unsigned int mask);
struct request_queue *blk_mq_init_queue(struct blk_mq_tag_set *set);
void blk_mq_virtio_map_queues(struct blk_mq_queue_map *qmap,
			      struct virtio_device *vdev, int first_vec);
void disk_set_zoned(struct gendisk *disk, int model);
struct scatterlist *sg_next(struct scatterlist *sg);
void virtqueue_disable_dma_api_for_buffers(struct virtqueue *vq);

void blk_queue_logical_block_size(struct request_queue *q, unsigned int size);
void blk_queue_physical_block_size(struct request_queue *q, unsigned int size);
void blk_queue_alignment_offset(struct request_queue *q, unsigned int offset);
void blk_queue_io_min(struct request_queue *q, unsigned int min);
void blk_queue_io_opt(struct request_queue *q, unsigned int opt);
void blk_queue_chunk_sectors(struct request_queue *q, unsigned int chunk_sectors);
void blk_queue_max_hw_sectors(struct request_queue *q, unsigned int max_hw_sectors);
void blk_queue_max_segments(struct request_queue *q, unsigned short max_segments);
void blk_queue_max_segment_size(struct request_queue *q, unsigned int max_size);
void blk_queue_max_discard_sectors(struct request_queue *q, unsigned int max_discard_sectors);
void blk_queue_max_discard_segments(struct request_queue *q, unsigned short max_discard_segments);
void blk_queue_max_secure_erase_sectors(struct request_queue *q, unsigned int max_sectors);
void blk_queue_max_write_zeroes_sectors(struct request_queue *q, unsigned int max_write_zeroes_sectors);
void blk_queue_max_zone_append_sectors(struct request_queue *q, unsigned int max_zone_append_sectors);
void blk_queue_write_cache(struct request_queue *q, bool enabled, bool fua);
bool mmc_can_gpio_cd(struct mmc_host *host);
void blk_queue_logical_block_size(struct request_queue *q, unsigned int size)
{
	q->limits.logical_block_size = size;

	if (q->limits.physical_block_size < size)
		q->limits.physical_block_size = size;

	if (q->limits.io_min < q->limits.physical_block_size)
		q->limits.io_min = q->limits.physical_block_size;

	q->limits.max_hw_sectors = round_down(q->limits.max_hw_sectors,
					    size >> SECTOR_SHIFT);
	q->limits.max_sectors = round_down(q->limits.max_sectors,
					  size >> SECTOR_SHIFT);
}
EXPORT_SYMBOL_GPL(blk_queue_logical_block_size);

void blk_queue_physical_block_size(struct request_queue *q, unsigned int size)
{
	q->limits.physical_block_size = size;

	if (q->limits.physical_block_size < q->limits.logical_block_size)
		q->limits.physical_block_size = q->limits.logical_block_size;

	if (q->limits.io_min < q->limits.physical_block_size)
		q->limits.io_min = q->limits.physical_block_size;
}
EXPORT_SYMBOL_GPL(blk_queue_physical_block_size);

void blk_queue_alignment_offset(struct request_queue *q, unsigned int offset)
{
	q->limits.alignment_offset =
		offset & (q->limits.physical_block_size - 1);
	q->limits.flags &= ~BLK_FLAG_MISALIGNED;
}
EXPORT_SYMBOL_GPL(blk_queue_alignment_offset);

void blk_queue_io_min(struct request_queue *q, unsigned int min)
{
	q->limits.io_min = min;

	if (q->limits.io_min < q->limits.logical_block_size)
		q->limits.io_min = q->limits.logical_block_size;

	if (q->limits.io_min < q->limits.physical_block_size)
		q->limits.io_min = q->limits.physical_block_size;
}
EXPORT_SYMBOL_GPL(blk_queue_io_min);

void blk_queue_io_opt(struct request_queue *q, unsigned int opt)
{
	q->limits.io_opt = opt;
}
EXPORT_SYMBOL_GPL(blk_queue_io_opt);

void blk_queue_chunk_sectors(struct request_queue *q, unsigned int chunk_sectors)
{
	q->limits.chunk_sectors = chunk_sectors;
}
EXPORT_SYMBOL_GPL(blk_queue_chunk_sectors);

void blk_queue_max_hw_sectors(struct request_queue *q, unsigned int max_hw_sectors)
{
	unsigned int min_max_hw_sectors = PAGE_SIZE >> SECTOR_SHIFT;
	unsigned int max_sectors;

	if (max_hw_sectors < min_max_hw_sectors)
		max_hw_sectors = min_max_hw_sectors;

	max_hw_sectors = round_down(max_hw_sectors,
				    q->limits.logical_block_size >> SECTOR_SHIFT);
	q->limits.max_hw_sectors = max_hw_sectors;

	max_sectors = min(max_hw_sectors, q->limits.max_dev_sectors);
	if (max_sectors > BLK_SAFE_MAX_SECTORS)
		max_sectors = BLK_SAFE_MAX_SECTORS;
	max_sectors = round_down(max_sectors,
			       q->limits.logical_block_size >> SECTOR_SHIFT);
	q->limits.max_sectors = max_sectors;
}
EXPORT_SYMBOL_GPL(blk_queue_max_hw_sectors);

void blk_queue_max_segments(struct request_queue *q, unsigned short max_segments)
{
	if (!max_segments)
		max_segments = 1;

	q->limits.max_segments = max_segments;
}
EXPORT_SYMBOL_GPL(blk_queue_max_segments);

void blk_queue_max_segment_size(struct request_queue *q, unsigned int max_size)
{
	unsigned int min_max_segment_size = PAGE_SIZE;

	if (max_size < min_max_segment_size)
		max_size = SECTOR_SIZE;

	q->limits.max_segment_size = max_size;
}
EXPORT_SYMBOL_GPL(blk_queue_max_segment_size);

void blk_queue_max_discard_sectors(struct request_queue *q,
				   unsigned int max_discard_sectors)
{
	q->limits.max_hw_discard_sectors = max_discard_sectors;
	q->limits.max_discard_sectors = max_discard_sectors;
}
EXPORT_SYMBOL_GPL(blk_queue_max_discard_sectors);

void blk_queue_max_discard_segments(struct request_queue *q,
				    unsigned short max_discard_segments)
{
	q->limits.max_discard_segments = max_discard_segments;
}
EXPORT_SYMBOL_GPL(blk_queue_max_discard_segments);

void blk_queue_max_secure_erase_sectors(struct request_queue *q,
					unsigned int max_sectors)
{
	q->limits.max_secure_erase_sectors = max_sectors;
}
EXPORT_SYMBOL_GPL(blk_queue_max_secure_erase_sectors);

void blk_queue_max_write_zeroes_sectors(struct request_queue *q,
					unsigned int max_write_zeroes_sectors)
{
	q->limits.max_write_zeroes_sectors = max_write_zeroes_sectors;
}
EXPORT_SYMBOL_GPL(blk_queue_max_write_zeroes_sectors);

void blk_queue_max_zone_append_sectors(struct request_queue *q,
				       unsigned int max_zone_append_sectors)
{
	unsigned int max_sectors;

	if (!blk_queue_is_zoned(q))
		return;

	max_sectors = min(q->limits.max_hw_sectors, max_zone_append_sectors);
	max_sectors = min(q->limits.chunk_sectors, max_sectors);
	q->limits.max_zone_append_sectors = max_sectors;
}
EXPORT_SYMBOL_GPL(blk_queue_max_zone_append_sectors);

/* 6.6 blk_queue_write_cache(q, wc, fua): 6.18 models it via features bits */
void blk_queue_write_cache(struct request_queue *q, bool enabled, bool fua)
{
	if (enabled)
		q->limits.features |= BLK_FEAT_WRITE_CACHE;
	else
		q->limits.features &= ~BLK_FEAT_WRITE_CACHE;

	if (fua)
		q->limits.features |= BLK_FEAT_FUA;
	else if (!(q->limits.features & BLK_FEAT_WRITE_CACHE))
		q->limits.features &= ~BLK_FEAT_FUA;
}
EXPORT_SYMBOL_GPL(blk_queue_write_cache);

/* 6.6 blk_queue_update_dma_pad(): MTK UFS tune; 6.18 dropped the knob.
 * The dma pad mask is derived by the DMA layer in 6.18; record the request
 * on the queue limits so the value is inspectable (no functional knob). */
void blk_queue_update_dma_pad(struct request_queue *q, unsigned int mask)
{
	q->limits.dma_alignment = mask;
}
EXPORT_SYMBOL_GPL(blk_queue_update_dma_pad);

struct scatterlist *sg_next(struct scatterlist *sg)
{
	return sg_next_618(sg);
}
EXPORT_SYMBOL(sg_next);

bool mmc_can_gpio_cd(struct mmc_host *host)
{
	return mmc_host_can_gpio_cd(host);
}
EXPORT_SYMBOL(mmc_can_gpio_cd);

/* 6.6 blk_mq_init_queue(set): 6.18 renamed to blk_mq_alloc_queue */
struct request_queue *blk_mq_init_queue(struct blk_mq_tag_set *set)
{
	return blk_mq_alloc_queue(set, NULL, NULL);
}
EXPORT_SYMBOL_GPL(blk_mq_init_queue);

/* 6.6 blk_mq_virtio_map_queues(): ported verbatim from 6.6 blk-mq-virtio.c */
void blk_mq_virtio_map_queues(struct blk_mq_queue_map *qmap,
			      struct virtio_device *vdev, int first_vec)
{
	const struct cpumask *mask;
	unsigned int queue, cpu;

	if (!vdev->config->get_vq_affinity)
		goto fallback;

	for (queue = 0; queue < qmap->nr_queues; queue++) {
		mask = vdev->config->get_vq_affinity(vdev, first_vec + queue);
		if (!mask)
			goto fallback;

		for_each_cpu(cpu, mask)
			qmap->mq_map[cpu] = qmap->queue_offset + queue;
	}

	return;

fallback:
	blk_mq_map_queues(qmap);
}
EXPORT_SYMBOL_GPL(blk_mq_virtio_map_queues);

/*
 * 6.6 disk_set_zoned(): 6.18 derives the zone model from the queue limits
 * passed at gendisk allocation, so there is no late knob to set.  On this
 * platform no zoned device exists (model == 0 == 6.6 BLK_ZONED_NONE), which
 * is the effective result; non-none requests are logged so a future zoned
 * backend is noticed.
 */
void disk_set_zoned(struct gendisk *disk, int model)
{
	if (WARN_ON_ONCE(model != 0)) {
		/* 6.18 flags zoned via queue_limits.features (BLK_FEAT_ZONED) */
		if (model > 0 && disk->queue)
			disk->queue->limits.features |= BLK_FEAT_ZONED;
		pr_warn("%s: zoned model %d mapped to 6.18 feature flag\n",
			disk->disk_name, model);
	}
}
EXPORT_SYMBOL_GPL(disk_set_zoned);

/*
 * 6.6 virtqueue_disable_dma_api_for_buffers(): 6.18 removed the per-vq
 * use_dma_api switch.  There is no late knob; log once so the condition is
 * visible (driver-visible effect: buffers stay DMA-mapped, matching the
 * always-DMA 6.18 model).
 */
void virtqueue_disable_dma_api_for_buffers(struct virtqueue *vq)
{
	static bool warned;

	if (!warned) {
		warned = true;
		pr_info("virtio: disable_dma_api_for_buffers not supported on 6.18; vq=%ps continues with DMA API\n",
			vq);
	}
}
EXPORT_SYMBOL_GPL(virtqueue_disable_dma_api_for_buffers);

/* ------------------------------------------------------------------ *
 * 第五轮：原型变了但厂商模块仍在按 6.6 调用（真机 zram panic 的根因）。
 * 6.18 的实现已改名为 *_k618，这里导出 6.6 原型。
 * ------------------------------------------------------------------ */
#include <linux/bsg-lib.h>

/* 6.18 头里为树内调用点加了重定向宏；这里要定义 6.6 原型的同名函数 */
#undef blk_rq_map_kern
#undef bsg_setup_queue

/* rodin-sig-protos: 6.6 原型（6.18 头文件里已改名，补声明避免
 * -Wmissing-prototypes；参数与 6.6 完全一致） */
struct gendisk *__blk_alloc_disk(int node, struct lock_class_key *lkclass);
struct gendisk *__blk_mq_alloc_disk(struct blk_mq_tag_set *set, void *queuedata,
				    struct lock_class_key *lkclass);
int blk_rq_map_kern(struct request_queue *q, struct request *rq, void *kbuf,
		    unsigned int len, gfp_t gfp_mask);
int __blk_rq_map_sg(struct request_queue *q, struct request *rq,
		    struct scatterlist *sglist, struct scatterlist **last_sg);
struct request_queue *bsg_setup_queue(struct device *dev, const char *name,
				      bsg_job_fn *job_fn, bsg_timeout_fn *timeout,
				      int dd_job_size);

/* 6.6 include/linux/blkdev.h: __blk_alloc_disk(int node, lkclass)
 * 6.18: __blk_alloc_disk(lim, node, lkclass) —— 多出的 lim 传 NULL 即 6.6 默认
 * （6.6 的 gendisk 没有 queue_limits 入参，默认限制由 blk_set_default_limits 给）。 */
struct gendisk *__blk_alloc_disk(int node, struct lock_class_key *lkclass)
{
	return __blk_alloc_disk_k618(NULL, node, lkclass);
}
EXPORT_SYMBOL(__blk_alloc_disk);

/* 6.6 blk-mq.h: __blk_mq_alloc_disk(set, queuedata, lkclass)
 * 6.18: __blk_mq_alloc_disk(set, lim, queuedata, lkclass) */
struct gendisk *__blk_mq_alloc_disk(struct blk_mq_tag_set *set, void *queuedata,
				    struct lock_class_key *lkclass)
{
	return __blk_mq_alloc_disk_k618(set, NULL, queuedata, lkclass);
}
EXPORT_SYMBOL(__blk_mq_alloc_disk);

/* 6.6 blk-mq.h: blk_rq_map_kern(q, rq, kbuf, len, gfp)
 * 6.18 用 rq->q，去掉首个 queue 参数。6.6 的 q 只是在 6.6 实现里做对齐检查用
 * （6.18 内部同样用 rq->q 做），所以直接转发。 */
int blk_rq_map_kern(struct request_queue *q, struct request *rq, void *kbuf,
		    unsigned int len, gfp_t gfp_mask)
{
	/* 6.6 用 q 的 limits 做校验；6.18 用 rq->q。二者在厂商调用点上同一个队列，
	 * 只是留个诊断：真不一致时按 6.18 的 rq->q 继续跑（不打断 I/O）。 */
	WARN_ON_ONCE(q && rq->q && q != rq->q);
	return blk_rq_map_kern_k618(rq, kbuf, len, gfp_mask);
}
EXPORT_SYMBOL(blk_rq_map_kern);

/* 6.6: __blk_rq_map_sg(q, rq, sglist, last_sg)；6.18 去掉首个 queue 参数 */
int __blk_rq_map_sg(struct request_queue *q, struct request *rq,
		    struct scatterlist *sglist, struct scatterlist **last_sg)
{
	WARN_ON_ONCE(q && rq->q && q != rq->q);
	return __blk_rq_map_sg_k618(rq, sglist, last_sg);
}
EXPORT_SYMBOL(__blk_rq_map_sg);

/* 6.6 bsg-lib.h: bsg_setup_queue(dev, name, job_fn, timeout, dd_job_size)
 * 6.18 在 name 后插入 queue_limits —— 传 NULL 表示用驱动自身的默认限制。 */
struct request_queue *bsg_setup_queue(struct device *dev, const char *name,
				      bsg_job_fn *job_fn, bsg_timeout_fn *timeout,
				      int dd_job_size)
{
	return bsg_setup_queue_k618(dev, name, NULL, job_fn, timeout, dd_job_size);
}
EXPORT_SYMBOL_GPL(bsg_setup_queue);
