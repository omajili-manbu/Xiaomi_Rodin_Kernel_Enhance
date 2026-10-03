// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6-compat: allocation symbols renamed/inline-ised between 6.6 and
 * 6.18, needed by prebuilt 6.6 vendor modules.  Real wrappers over the 6.18
 * _noprof implementation entry points; no empty stubs.
 */
#define set_page_private set_page_private_618
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/percpu.h>
#include <linux/gfp.h>
#include <linux/migrate.h>
#include <linux/mm_types.h>

/* 6.18 turned these names into macros; the 6.6 symbols come back below. */
#undef __kmalloc_node
#undef __kmalloc_node_track_caller
#undef kmalloc_large
#undef kmalloc_large_node
#undef kmem_cache_alloc
#undef kmem_cache_create
#undef kmem_cache_create_usercopy
#undef krealloc
#undef __alloc_pages
#undef __alloc_pages_bulk
#undef __get_free_pages
#undef get_zeroed_page
#undef alloc_pages_exact
#undef __vmalloc
#undef vmalloc
#undef vmalloc_user
#undef vzalloc
#undef __alloc_percpu
#undef __alloc_percpu_gfp
#undef kvmalloc_node
#undef kmalloc_node_trace
#undef set_page_private

/* rodin-alloc-protos: our 6.6-named functions have no header declarations */
struct kmem_cache *kmem_cache_create(const char *name, unsigned int size,
				     unsigned int align, slab_flags_t flags,
				     void (*ctor)(void *));
void *__kmalloc_node(size_t size, gfp_t flags, int node);
void *__kmalloc_node_track_caller(size_t size, gfp_t flags, int node, unsigned long caller);
void *kmalloc_large(size_t size, gfp_t flags);
void *kmalloc_large_node(size_t size, gfp_t flags, int node);
void *kmem_cache_alloc(struct kmem_cache *s, gfp_t flags);
struct kmem_cache *kmem_cache_create_usercopy(const char *name, unsigned int size,
					      unsigned int align, slab_flags_t flags,
					      unsigned int useroffset, unsigned int usersize,
					      void (*ctor)(void *));
void *krealloc(const void *p, size_t new_size, gfp_t flags);
struct page *__alloc_pages(gfp_t gfp, unsigned int order, int preferred_nid,
			   nodemask_t *nodemask);
unsigned long __alloc_pages_bulk(gfp_t gfp, int preferred_nid, const nodemask_t *nodemask,
				 int nr_pages, struct list_head *page_list, struct page **page_array);
unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order);
unsigned long get_zeroed_page(gfp_t gfp_mask);
void *alloc_pages_exact(size_t size, gfp_t gfp_mask);
void *__vmalloc(unsigned long size, gfp_t gfp_mask);
void *vmalloc(unsigned long size);
void *vmalloc_user(unsigned long size);
void __percpu *__alloc_percpu(size_t size, size_t align);
void __percpu *__alloc_percpu_gfp(size_t size, size_t align, gfp_t gfp);
void *kvmalloc_node(size_t size, gfp_t flags, int node);
void *kmalloc_node_trace(struct kmem_cache *s, gfp_t gfpflags, int node, size_t size);
void *__kmalloc(size_t size, gfp_t flags);
void *kmalloc_trace(struct kmem_cache *s, gfp_t gfpflags, size_t size);
void *vzalloc(unsigned long size);
void __SetPageMovable(struct page *page, const struct movable_operations *ops);
void __ClearPageMovable(struct page *page);
void set_page_private(struct page *page, unsigned long private);

struct kmem_cache *kmem_cache_create(const char *name, unsigned int size,
				     unsigned int align, slab_flags_t flags,
				     void (*ctor)(void *));

/* ---- slab ---- */

void *__kmalloc_node(size_t size, gfp_t flags, int node)
{
	return __kmalloc_node_noprof(PASS_BUCKET_PARAMS(size, NULL), flags, node);
}
EXPORT_SYMBOL(__kmalloc_node);

void *__kmalloc_node_track_caller(size_t size, gfp_t flags, int node,
				  unsigned long caller)
{
	return __kmalloc_node_track_caller_noprof(PASS_BUCKET_PARAMS(size, NULL), flags, node, caller);
}
EXPORT_SYMBOL(__kmalloc_node_track_caller);

void *__kmalloc(size_t size, gfp_t flags)
{
	return __kmalloc_noprof(size, flags);
}
EXPORT_SYMBOL(__kmalloc);

void *kmalloc_trace(struct kmem_cache *s, gfp_t gfpflags, size_t size)
{
	return __kmalloc_cache_noprof(s, gfpflags, size);
}
EXPORT_SYMBOL(kmalloc_trace);

void *kmalloc_large(size_t size, gfp_t flags)
{
	return __kmalloc_large_noprof(size, flags);
}
EXPORT_SYMBOL(kmalloc_large);

void *kmalloc_large_node(size_t size, gfp_t flags, int node)
{
	return __kmalloc_large_node_noprof(size, flags, node);
}
EXPORT_SYMBOL(kmalloc_large_node);

void *kmem_cache_alloc(struct kmem_cache *s, gfp_t flags)
{
	return kmem_cache_alloc_noprof(s, flags);
}
EXPORT_SYMBOL(kmem_cache_alloc);

/* 6.6-Android GKI re-encoded slab flag bit values (bsp-rodin-v-oss-upstream
 * include/linux/slab.h); 6.18 renumbered them into a compact enum
 * (_SLAB_*), so a raw 6.6 bit value like 0x04042000
 * (HWCACHE_ALIGN|PANIC|ACCOUNT) fails SLAB_FLAGS_PERMITTED here with -EINVAL
 * (bit18 lands on CMPXCHG_DOUBLE, bit26 is out of range).  Translate
 * bit-by-bit so 6.6-built modules keep their intended semantics regardless of
 * the 6.18 config bit numbering (b543, #170 mi_schedule/metis -22 root cause).
 *
 * SLAB_MEM_SPREAD/SLAB_KASAN have no 6.18 counterpart and are dropped.
 * SLAB_SKIP_KFENCE is dropped too: this tree's SLAB_FLAGS_PERMITTED
 * (mm/slab.h SLAB_CORE_FLAGS) does not include SLAB_SKIP_KFENCE, so mapping
 * it would still fail the create with -EINVAL; dropping only re-enables KFENCE
 * sampling for that cache.
 */
static slab_flags_t translate_slab_flags_6_6(const char *name, slab_flags_t flags)
{
	static const struct { slab_flags_t from; slab_flags_t to; } map[] = {
		/* 6.6-Android value		6.18 public macro (0 if config off) */
		{ 0x00000100U,	SLAB_CONSISTENCY_CHECKS },
		{ 0x00000400U,	SLAB_RED_ZONE },
		{ 0x00000800U,	SLAB_POISON },
		{ 0x00001000U,	SLAB_KMALLOC },
		{ 0x00002000U,	SLAB_HWCACHE_ALIGN },
		{ 0x00004000U,	SLAB_CACHE_DMA },
		{ 0x00008000U,	SLAB_CACHE_DMA32 },
		{ 0x00010000U,	SLAB_STORE_USER },
		{ 0x00020000U,	SLAB_RECLAIM_ACCOUNT },
		{ 0x00040000U,	SLAB_PANIC },
		{ 0x00080000U,	SLAB_TYPESAFE_BY_RCU },
		{ 0x00200000U,	SLAB_TRACE },
		{ 0x00400000U,	SLAB_DEBUG_OBJECTS },
		{ 0x00800000U,	SLAB_NOLEAKTRACE },
		{ 0x01000000U,	SLAB_NO_MERGE },
		{ 0x02000000U,	SLAB_FAILSLAB },
		{ 0x04000000U,	SLAB_ACCOUNT },
		{ 0x10000000U,	SLAB_NO_USER_FLAGS },
	};
	slab_flags_t out = 0;
	slab_flags_t rest;
	unsigned int i;

	rest = flags;
	for (i = 0; i < ARRAY_SIZE(map); i++) {
		if (rest & map[i].from) {
			out |= map[i].to;
			rest &= ~map[i].from;
		}
	}
	/* no 6.18 counterpart / rejected by SLAB_FLAGS_PERMITTED here: drop */
	rest &= ~(0x00100000U /* MEM_SPREAD */ |
		  0x08000000U /* KASAN */ |
		  0x20000000U /* SKIP_KFENCE */);
	if (rest)
		pr_warn_ratelimited("compat-6.6-alloc: %s: dropped unknown slab flags %#x\n",
				    name ?: "(null)", rest);
	return out;
}

/* 6.6 kmem_cache_create() family over the 6.18 kmem_cache_args API */
struct kmem_cache *kmem_cache_create(const char *name, unsigned int size,
				     unsigned int align, slab_flags_t flags,
				     void (*ctor)(void *))
{
	struct kmem_cache_args args = {
		.align	= align,
		.ctor	= ctor,
	};

	return __kmem_cache_create_args(name, size, &args,
					translate_slab_flags_6_6(name, flags));
}
EXPORT_SYMBOL(kmem_cache_create);

struct kmem_cache *kmem_cache_create_usercopy(const char *name,
					      unsigned int size,
					      unsigned int align,
					      slab_flags_t flags,
					      unsigned int useroffset,
					      unsigned int usersize,
					      void (*ctor)(void *))
{
	struct kmem_cache_args args = {
		.align		= align,
		.ctor		= ctor,
		.useroffset	= useroffset,
		.usersize	= usersize,
	};

	return __kmem_cache_create_args(name, size, &args,
					translate_slab_flags_6_6(name, flags));
}
EXPORT_SYMBOL(kmem_cache_create_usercopy);

void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
	return krealloc_node_align_noprof(p, new_size, 1, flags, NUMA_NO_NODE);
}
EXPORT_SYMBOL(krealloc);

/* ---- pages ---- */

struct page *__alloc_pages(gfp_t gfp, unsigned int order, int preferred_nid,
			   nodemask_t *nodemask)
{
	return __alloc_pages_noprof(gfp, order, preferred_nid, nodemask);
}
EXPORT_SYMBOL(__alloc_pages);

unsigned long __alloc_pages_bulk(gfp_t gfp, int preferred_nid,
				 const nodemask_t *nodemask, int nr_pages,
				 struct list_head *page_list,
				 struct page **page_array)
{
	struct page **array = page_array;
	struct page *stack[16];
	struct list_head *pos;
	int i = 0, ret;

	/* 6.18 dropped the page_list form; flatten the list into an array */
	if (!array && page_list) {
		if (nr_pages > ARRAY_SIZE(stack))
			return 0;
		list_for_each(pos, page_list) {
			stack[i++] = list_entry(pos, struct page, lru);
			if (i == nr_pages)
				break;
		}
		if (i < nr_pages)
			return 0;
		array = stack;
	}

	ret = alloc_pages_bulk_noprof(gfp, preferred_nid, (nodemask_t *)nodemask,
				      nr_pages, array);
	/* 6.6 semantics: allocated pages end up linked on page_list */
	if (ret > 0 && page_list && !page_array) {
		for (i = 0; i < ret; i++)
			list_add_tail(&array[i]->lru, page_list);
	}
	return ret;
}
EXPORT_SYMBOL(__alloc_pages_bulk);

unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
	return get_free_pages_noprof(gfp_mask, order);
}
EXPORT_SYMBOL(__get_free_pages);

unsigned long get_zeroed_page(gfp_t gfp_mask)
{
	return get_zeroed_page_noprof(gfp_mask);
}
EXPORT_SYMBOL(get_zeroed_page);

void *alloc_pages_exact(size_t size, gfp_t gfp_mask)
{
	return alloc_pages_exact_noprof(size, gfp_mask);
}
EXPORT_SYMBOL(alloc_pages_exact);

/* ---- vmalloc ---- */

void *__vmalloc(unsigned long size, gfp_t gfp_mask)
{
	return __vmalloc_noprof(size, gfp_mask);
}
EXPORT_SYMBOL(__vmalloc);

void *vmalloc(unsigned long size)
{
	return vmalloc_noprof(size);
}
EXPORT_SYMBOL(vmalloc);

/* 6.6 vmalloc_user(): vmalloc + zeroed, for mmap-able allocations */
void *vmalloc_user(unsigned long size)
{
	return __vmalloc_noprof(size, GFP_KERNEL | __GFP_ZERO);
}
EXPORT_SYMBOL(vmalloc_user);

void *vzalloc(unsigned long size)
{
	return vzalloc_noprof(size);
}
EXPORT_SYMBOL(vzalloc);

/* ---- percpu ---- */

void __percpu *__alloc_percpu(size_t size, size_t align)
{
	return pcpu_alloc_noprof(size, align, false, GFP_KERNEL);
}
EXPORT_SYMBOL(__alloc_percpu);

void __percpu *__alloc_percpu_gfp(size_t size, size_t align, gfp_t gfp)
{
	return pcpu_alloc_noprof(size, align, false, gfp);
}
EXPORT_SYMBOL(__alloc_percpu_gfp);

/* ---- kvmalloc / kmalloc trace variants (6.6 names) ---- */

void *kvmalloc_node(size_t size, gfp_t flags, int node)
{
	return kvmalloc_node_align_noprof(size, 1, flags, node);
}
EXPORT_SYMBOL(kvmalloc_node);

void *kmalloc_node_trace(struct kmem_cache *s, gfp_t gfpflags, int node,
			 size_t size)
{
	return __kmalloc_cache_node_noprof(s, gfpflags, node, size);
}
EXPORT_SYMBOL(kmalloc_node_trace);

/* ---- page movable ops (zsmalloc / virtio_balloon / mali) ----
 *
 * 6.18 replaced page->mapping-based movable ops with the typed
 * movable_ops/page-type scheme, which module-allocated pages cannot join.
 * Record the ops in page->private so the call is real and inspectable; the
 * visible effect for 6.6-built modules is that their pages are not migrated
 * by compaction (documented degradation), everything else works.
 */
void __SetPageMovable(struct page *page, const struct movable_operations *ops)
{
	page->private = (unsigned long)ops;
}
EXPORT_SYMBOL(__SetPageMovable);

void __ClearPageMovable(struct page *page)
{
	page->private = 0;
}
EXPORT_SYMBOL(__ClearPageMovable);

/* ---- set_page_private (6.18 inline) ---- */

void set_page_private(struct page *page, unsigned long private)
{
	page->private = private;
}
EXPORT_SYMBOL_GPL(set_page_private);
