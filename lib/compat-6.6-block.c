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
#undef unregister_shrinker

void blk_queue_logical_block_size(struct request_queue *q, unsigned int size)
{
	blk_queue_logical_block_size_618(q, size);
}
EXPORT_SYMBOL_GPL(blk_queue_logical_block_size);

void blk_queue_physical_block_size(struct request_queue *q, unsigned int size)
{
	blk_queue_physical_block_size_618(q, size);
}
EXPORT_SYMBOL_GPL(blk_queue_physical_block_size);

void blk_queue_alignment_offset(struct request_queue *q, unsigned int offset)
{
	blk_queue_alignment_offset_618(q, offset);
}
EXPORT_SYMBOL_GPL(blk_queue_alignment_offset);

void blk_queue_io_min(struct request_queue *q, unsigned int min)
{
	blk_queue_io_min_618(q, min);
}
EXPORT_SYMBOL_GPL(blk_queue_io_min);

void blk_queue_io_opt(struct request_queue *q, unsigned int opt)
{
	blk_queue_io_opt_618(q, opt);
}
EXPORT_SYMBOL_GPL(blk_queue_io_opt);

void blk_queue_chunk_sectors(struct request_queue *q, unsigned int chunk_sectors)
{
	blk_queue_chunk_sectors_618(q, chunk_sectors);
}
EXPORT_SYMBOL_GPL(blk_queue_chunk_sectors);

void blk_queue_max_hw_sectors(struct request_queue *q, unsigned int max_hw_sectors)
{
	blk_queue_max_hw_sectors_618(q, max_hw_sectors);
}
EXPORT_SYMBOL_GPL(blk_queue_max_hw_sectors);

void blk_queue_max_segments(struct request_queue *q, unsigned short max_segments)
{
	blk_queue_max_segments_618(q, max_segments);
}
EXPORT_SYMBOL_GPL(blk_queue_max_segments);

void blk_queue_max_segment_size(struct request_queue *q, unsigned int max_size)
{
	blk_queue_max_segment_size_618(q, max_size);
}
EXPORT_SYMBOL_GPL(blk_queue_max_segment_size);

void blk_queue_max_discard_sectors(struct request_queue *q,
				   unsigned int max_discard_sectors)
{
	blk_queue_max_discard_sectors_618(q, max_discard_sectors);
}
EXPORT_SYMBOL_GPL(blk_queue_max_discard_sectors);

void blk_queue_max_discard_segments(struct request_queue *q,
				    unsigned short max_discard_segments)
{
	blk_queue_max_discard_segments_618(q, max_discard_segments);
}
EXPORT_SYMBOL_GPL(blk_queue_max_discard_segments);

void blk_queue_max_secure_erase_sectors(struct request_queue *q,
					unsigned int max_sectors)
{
	blk_queue_max_secure_erase_sectors_618(q, max_sectors);
}
EXPORT_SYMBOL_GPL(blk_queue_max_secure_erase_sectors);

void blk_queue_max_write_zeroes_sectors(struct request_queue *q,
					unsigned int max_write_zeroes_sectors)
{
	blk_queue_max_write_zeroes_sectors_618(q, max_write_zeroes_sectors);
}
EXPORT_SYMBOL_GPL(blk_queue_max_write_zeroes_sectors);

void blk_queue_max_zone_append_sectors(struct request_queue *q,
				       unsigned int max_zone_append_sectors)
{
	blk_queue_max_zone_append_sectors_618(q, max_zone_append_sectors);
}
EXPORT_SYMBOL_GPL(blk_queue_max_zone_append_sectors);

/* 6.6 blk_queue_write_cache(q, wc, fua): 6.18 models it as queue limits */
void blk_queue_write_cache(struct request_queue *q, bool enabled, bool fua)
{
	blk_queue_write_cache_618(q, enabled, fua);
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
	return mmc_can_gpio_cd_618(host);
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
 * platform no zoned device exists (BLK_ZONED_NONE), which is the effective
 * result; non-none requests are logged so a future zoned backend is noticed.
 */
void disk_set_zoned(struct gendisk *disk, enum blk_zoned_model model)
{
	if (WARN_ON_ONCE(model != BLK_ZONED_NONE))
		pr_warn("%s: zoned model %d ignored (no 6.18 late knob)
",
			disk->disk_name, model);
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
		pr_info("virtio: disable_dma_api_for_buffers not supported on 6.18; vq=%ps continues with DMA API
",
			vq);
	}
}
EXPORT_SYMBOL_GPL(virtqueue_disable_dma_api_for_buffers);
