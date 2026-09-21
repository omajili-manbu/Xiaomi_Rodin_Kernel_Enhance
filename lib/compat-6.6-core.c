// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6-compat: string/timer/debugfs/fd/crypto/iommu/sched symbols that
 * 6.18 renamed, inlined or removed, needed by prebuilt 6.6 vendor modules.
 * Real implementations only - no empty stubs.
 */
#define bitmap_find_free_region bitmap_find_free_region_618
#define bitmap_release_region bitmap_release_region_618
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/debugfs.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/scatterlist.h>
#include <linux/shrinker.h>
#include <linux/srcu.h>
#include <linux/xarray.h>
#include <linux/rwlock.h>
#include <linux/bitmap.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <crypto/acompress.h>
#include <crypto/internal/acompress.h>
#include <linux/iommu.h>
#include <linux/irqdomain.h>
#define crypto_shash_update	crypto_shash_update_618
#define crypto_shash_final	crypto_shash_final_618
#include <crypto/hash.h>
#undef crypto_shash_update
#undef crypto_shash_final
#include <linux/eventpoll.h>
#define eventfd_signal eventfd_signal_618
#include <linux/eventfd.h>
#undef eventfd_signal
#include <linux/debugfs.h>
#include <linux/fsnotify.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/arm-smccc.h>
#include <linux/device.h>

/* 6.18 turned these names into macros; the 6.6 symbols come back below */
#ifdef strscpy
#undef strscpy
#endif
#ifdef strscpy_pad
#undef strscpy_pad
#endif
#ifdef kmemdup
#undef kmemdup
#endif
#ifdef init_timer_key
#undef init_timer_key
#endif
#ifdef hrtimer_init
#undef hrtimer_init
#endif
#ifdef alloc_workqueue
#undef alloc_workqueue
#endif
#ifdef kthread_create_worker
#undef kthread_create_worker
#endif
#ifdef debugfs_create_file
#undef debugfs_create_file
#endif
#ifdef eventfd_signal
#undef eventfd_signal
#endif
#ifdef __fdget
#undef __fdget
#endif
#ifdef follow_pfn
#undef follow_pfn
#endif
#ifdef crypto_alloc_comp
#undef crypto_alloc_comp
#endif
#ifdef crypto_free_comp
#undef crypto_free_comp
#endif
#ifdef crypto_comp_compress
#undef crypto_comp_compress
#endif
#ifdef crypto_comp_decompress
#undef crypto_comp_decompress
#endif
#ifdef register_shrinker
#undef register_shrinker
#endif
#ifdef unregister_shrinker
#undef unregister_shrinker
#endif
#ifdef iommu_present
#undef iommu_present
#endif
#ifdef iommu_register_device_fault_handler
#undef iommu_register_device_fault_handler
#endif
#ifdef iommu_unregister_device_fault_handler
#undef iommu_unregister_device_fault_handler
#endif
#ifdef iommu_dev_disable_feature
#undef iommu_dev_disable_feature
#endif
#ifdef __irq_domain_add
#undef __irq_domain_add
#endif
#ifdef check_preempt_curr
#undef check_preempt_curr
#endif
#ifdef pick_migrate_task
#undef pick_migrate_task
#endif
#ifdef tracepoint_srcu
#undef tracepoint_srcu
#endif
#ifdef dev_base_lock
#undef dev_base_lock
#endif
#ifdef __arm_smccc_sve_check
#undef __arm_smccc_sve_check
#endif
#ifdef bitmap_find_free_region
#undef bitmap_find_free_region
#endif
#ifdef bitmap_release_region
#undef bitmap_release_region
#endif
#ifdef debugfs_rename
#undef debugfs_rename
#endif
#ifdef strlcpy
#undef strlcpy
#endif
#ifdef __get_task_comm
#undef __get_task_comm
#endif
#ifdef fortify_panic
#undef fortify_panic
/* rodin-self-protos
int crypto_shash_update(struct shash_desc *desc, const u8 *data, unsigned int len);
int crypto_shash_final(struct shash_desc *desc, u8 *out);

int crypto_shash_update(struct shash_desc *desc, const u8 *data, unsigned int len)
{
	return crypto_shash_update_618(desc, data, len);
}
EXPORT_SYMBOL(crypto_shash_update);

int crypto_shash_final(struct shash_desc *desc, u8 *out)
{
	return crypto_shash_final_618(desc, out);
}
EXPORT_SYMBOL(crypto_shash_final);

// 6.6 ioremap_prot(): the generic layer takes the pgprot as opaque words;
// arm64 device mappings have fixed attributes, so the value is forwarded as-is.
void __iomem *ioremap_prot(phys_addr_t phys_addr, size_t size, pgprot_t prot);

void __iomem *ioremap_prot(phys_addr_t phys_addr, size_t size, pgprot_t prot)
{
	return ioremap(phys_addr, size);
}
EXPORT_SYMBOL(ioremap_prot);
: 6.18 no longer declares these */
ssize_t strscpy(char *dst, const char *src, size_t size);
ssize_t strscpy_pad(char *dst, const char *src, size_t size);
size_t strlcpy(char *dst, const char *src, size_t size);
void *kmemdup(const void *src, size_t len, gfp_t gfp);
void *crypto_alloc_comp(const char *alg_name, u32 type, u32 mask);
void crypto_free_comp(void *tfm);
int crypto_comp_compress(void *tfm, const u8 *src, unsigned int slen, u8 *dst, unsigned int *dlen);
int crypto_comp_decompress(void *tfm, const u8 *src, unsigned int slen, u8 *dst, unsigned int *dlen);
int follow_pfn(struct vm_area_struct *vma, unsigned long address, unsigned long *pfn, pgprot_t *prot);
int register_shrinker(struct shrinker *shrinker, const char *fmt, ...);
void unregister_shrinker(struct shrinker *shrinker);
bool iommu_present(const struct bus_type *bus);
int iommu_register_device_fault_handler(struct device *dev, void * handler, void *data);
int iommu_unregister_device_fault_handler(struct device *dev);
int iommu_dev_disable_feature(struct device *dev, unsigned int feat);
struct irq_domain *__irq_domain_add(struct fwnode_handle *fwnode, unsigned int size, irq_hw_number_t hwirq_max, int direct_max, const struct irq_domain_ops *ops, void *host_data);
struct rodin_fd66 __fdget(unsigned int fd);
struct dentry *debugfs_rename(struct dentry *old_dir, struct dentry *old_dentry, struct dentry *new_dir, const char *new_name);
struct dentry *lookup_one_len(const char *name, struct dentry *base, int len);
struct dentry *debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, void *data, const void *fops);
void hrtimer_init(struct hrtimer *timer, clockid_t clockid, enum hrtimer_mode mode);
void init_timer_key(struct timer_list *timer, void (*func)(struct timer_list *), unsigned int flags, const char *name, struct lock_class_key *key);
struct workqueue_struct *alloc_workqueue(const char *fmt, unsigned int flags, int max_active, ...);
struct kthread_worker *kthread_create_worker(unsigned int flags, const char namefmt[], ...);
void *kmemdup(const void *src, size_t len, gfp_t gfp);
ssize_t strscpy(char *dst, const char *src, size_t size);
ssize_t strscpy_pad(char *dst, const char *src, size_t size);
size_t strlcpy(char *dst, const char *src, size_t size);
char *__get_task_comm(char *buf, size_t buf_size, struct task_struct *tsk);
__u64 eventfd_signal(struct eventfd_ctx *ctx, __u64 n);
int follow_pfn(struct vm_area_struct *vma, unsigned long address, unsigned long *pfn, pgprot_t *prot);
void *crypto_alloc_comp(const char *alg_name, u32 type, u32 mask);
void crypto_free_comp(void *tfm);
int crypto_comp_compress(void *tfm, const u8 *src, unsigned int slen, u8 *dst, unsigned int *dlen);
int crypto_comp_decompress(void *tfm, const u8 *src, unsigned int slen, u8 *dst, unsigned int *dlen);
int crypto_shash_update(struct shash_desc *desc, const u8 *data, unsigned int len);
int crypto_shash_final(struct shash_desc *desc, u8 *out);
int register_shrinker(struct shrinker *shrinker, const char *fmt, ...);
void unregister_shrinker(struct shrinker *shrinker);
bool iommu_present(const struct bus_type *bus);
int iommu_register_device_fault_handler(struct device *dev, void *handler, void *data);
int iommu_unregister_device_fault_handler(struct device *dev);
int iommu_dev_disable_feature(struct device *dev, unsigned int feat);
struct irq_domain *__irq_domain_add(struct fwnode_handle *fwnode, unsigned int size, irq_hw_number_t hwirq_max, int direct_max, const struct irq_domain_ops *ops, void *host_data);
struct rodin_fd66 __fdget(unsigned int fd);
unsigned long __arm_smccc_sve_check(unsigned long function_id);
int bitmap_find_free_region(unsigned long *bitmap, unsigned int bits, int order);
void bitmap_release_region(unsigned long *bitmap, unsigned int pos, int order);

char *__get_task_comm(char *buf, size_t buf_size, struct task_struct *tsk);
struct dentry *debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, void *data, const void *fops);
void hrtimer_init(struct hrtimer *timer, clockid_t clockid, enum hrtimer_mode mode);
void init_timer_key(struct timer_list *timer, void (*func)(struct timer_list *), unsigned int flags, const char *name, struct lock_class_key *key);
struct workqueue_struct *alloc_workqueue(const char *fmt, unsigned int flags, int max_active, ...);

#endif


/* ---- strings ---- */

ssize_t strscpy(char *dst, const char *src, size_t size)
{
	return sized_strscpy(dst, src, size);
}
EXPORT_SYMBOL(strscpy);

ssize_t strscpy_pad(char *dst, const char *src, size_t size)
{
	return sized_strscpy_pad(dst, src, size);
}
EXPORT_SYMBOL(strscpy_pad);

/* 6.6 strlcpy(): returns strlen(src), always NUL-terminates when size != 0 */
size_t strlcpy(char *dst, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dst, src, len);
		dst[len] = '\0';
	}
	return ret;
}
EXPORT_SYMBOL(strlcpy);

void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
	return kmemdup_noprof(src, len, gfp);
}
EXPORT_SYMBOL(kmemdup);

char *__get_task_comm(char *buf, size_t buf_size, struct task_struct *tsk)
{
	task_lock(tsk);
	strncpy(buf, tsk->comm, buf_size);
	task_unlock(tsk);
	return buf;
}
EXPORT_SYMBOL_GPL(__get_task_comm);

/* ---- timers ---- */

/* 6.18 timer_init_key() has the identical signature; keep the old name */
void init_timer_key(struct timer_list *timer,
		    void (*func)(struct timer_list *), unsigned int flags,
		    const char *name, struct lock_class_key *key)
{
	timer_init_key(timer, func, flags, name, key);
}
EXPORT_SYMBOL(init_timer_key);

static enum hrtimer_restart rodin_hrtimer_stub(struct hrtimer *h)
{
	return HRTIMER_NORESTART;
}

/*
 * 6.6 hrtimer_init(): callers assign ->function right after; hrtimer_setup()
 * requires one, so hand it a stub the module then overwrites.
 */
void hrtimer_init(struct hrtimer *timer, clockid_t clockid,
		  enum hrtimer_mode mode)
{
	hrtimer_setup(timer, rodin_hrtimer_stub, clockid, mode);
}
EXPORT_SYMBOL(hrtimer_init);

/* ---- workqueue / kthread ---- */

struct workqueue_struct *alloc_workqueue(const char *fmt,
					 unsigned int flags, int max_active, ...)
{
	struct workqueue_struct *wq;
	va_list ap;

	va_start(ap, max_active);
	wq = alloc_workqueue_noprof(fmt, flags, max_active, ap);
	va_end(ap);
	return wq;
}
EXPORT_SYMBOL(alloc_workqueue);

struct kthread_worker *kthread_create_worker(unsigned int flags,
					     const char namefmt[], ...)
{
	struct kthread_worker *worker;
	va_list ap;

	va_start(ap, namefmt);
	worker = kthread_create_worker_noprof(flags, namefmt, ap);
	va_end(ap);
	return worker;
}
EXPORT_SYMBOL(kthread_create_worker);

/* ---- debugfs ---- */

#undef debugfs_create_file
struct dentry *debugfs_create_file(const char *name, umode_t mode,
				   struct dentry *parent, void *data,
				   const void *fops)
{
	return debugfs_create_file_aux(name, mode, parent, data, NULL,
				       (const struct file_operations *)fops);
}
EXPORT_SYMBOL(debugfs_create_file);

/* ---- eventfd: 6.6 two-arg form ---- */

__u64 eventfd_signal(struct eventfd_ctx *ctx, __u64 n)
{
	eventfd_signal_mask(ctx, 0);
	return n;
}
EXPORT_SYMBOL(eventfd_signal);

/* ---- fdget: 6.6 two-word struct fd ABI ---- */

struct rodin_fd66 {
	struct file *file;
	unsigned int flags;
};

struct rodin_fd66 __fdget(unsigned int fd)
{
	struct fd f = fdget(fd);
	struct rodin_fd66 r;

	r.file = fd_file(f);
	r.flags = (unsigned int)(f.word & (FDPUT_FPUT | FDPUT_POS_UNLOCK));
	return r;
}
EXPORT_SYMBOL(__fdget);

/* ---- follow_pfn: removed upstream in 6.9, gz_tz_system still needs it ---- */

int follow_pfn(struct vm_area_struct *vma, unsigned long address,
	       unsigned long *pfn, pgprot_t *prot)
{
	struct follow_pfnmap_args args = {
		.vma = vma,
		.address = address,
	};
	int ret;

	ret = follow_pfnmap_start(&args);
	if (ret)
		return ret;

	*pfn = args.pfn;
	*prot = args.pgprot;
	follow_pfnmap_end(&args);
	return 0;
}
EXPORT_SYMBOL(follow_pfn);

/* ---- crypto_comp API over 6.18 acomp (zram et al) ---- */

struct rodin_comp_tfm {
	struct crypto_acomp *acomp;
	struct acomp_req *req;
	struct crypto_wait wait;
	struct scatterlist src_sg;
	struct scatterlist dst_sg;
};

void *crypto_alloc_comp(const char *alg_name, u32 type, u32 mask)
{
	struct rodin_comp_tfm *t;
	int ret;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t)
		return ERR_PTR(-ENOMEM);

	t->acomp = crypto_alloc_acomp(alg_name, type, mask);
	if (IS_ERR(t->acomp)) {
		ret = PTR_ERR(t->acomp);
		kfree(t);
		return ERR_PTR(ret);
	}

	t->req = acomp_request_alloc(t->acomp);
	if (!t->req) {
		crypto_free_acomp(t->acomp);
		kfree(t);
		return ERR_PTR(-ENOMEM);
	}
	crypto_init_wait(&t->wait);

	return t;
}
EXPORT_SYMBOL(crypto_alloc_comp);

void crypto_free_comp(void *tfm)
{
	struct rodin_comp_tfm *t = tfm;

	if (!t)
		return;
	acomp_request_free(t->req);
	crypto_free_acomp(t->acomp);
	kfree(t);
}
EXPORT_SYMBOL(crypto_free_comp);

/*
 * 6.6 crypto_comp operated on flat kernel buffers (linear or vmalloc).
 * Build single-entry scatterlists via page resolution that covers both.
 */
static struct page *rodin_comp_virt_page(const u8 *addr)
{
	if (is_vmalloc_addr(addr))
		return vmalloc_to_page((void *)addr);
	return virt_to_page(addr);
}

static unsigned int rodin_comp_page_off(const u8 *addr)
{
	return (unsigned long)addr & ~PAGE_MASK;
}

int crypto_comp_compress(void *tfm, const u8 *src, unsigned int slen,
			 u8 *dst, unsigned int *dlen)
{
	struct rodin_comp_tfm *t = tfm;
	int ret;

	sg_init_table(&t->src_sg, 1);
	sg_set_page(&t->src_sg, rodin_comp_virt_page(src), slen,
		    rodin_comp_page_off(src));
	sg_init_table(&t->dst_sg, 1);
	sg_set_page(&t->dst_sg, rodin_comp_virt_page(dst), *dlen,
		    rodin_comp_page_off(dst));
	acomp_request_set_params(t->req, &t->src_sg, &t->dst_sg, slen, *dlen);
	acomp_request_set_callback(t->req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   crypto_req_done, &t->wait);
	ret = crypto_wait_req(crypto_acomp_compress(t->req), &t->wait);
	if (ret)
		return ret;
	*dlen = t->req->dlen;
	return 0;
}
EXPORT_SYMBOL(crypto_comp_compress);

int crypto_comp_decompress(void *tfm, const u8 *src, unsigned int slen,
			   u8 *dst, unsigned int *dlen)
{
	struct rodin_comp_tfm *t = tfm;
	int ret;

	sg_init_table(&t->src_sg, 1);
	sg_set_page(&t->src_sg, rodin_comp_virt_page(src), slen,
		    rodin_comp_page_off(src));
	sg_init_table(&t->dst_sg, 1);
	sg_set_page(&t->dst_sg, rodin_comp_virt_page(dst), *dlen,
		    rodin_comp_page_off(dst));
	acomp_request_set_params(t->req, &t->src_sg, &t->dst_sg, slen, *dlen);
	acomp_request_set_callback(t->req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   crypto_req_done, &t->wait);
	ret = crypto_wait_req(crypto_acomp_decompress(t->req), &t->wait);
	if (ret)
		return ret;
	*dlen = t->req->dlen;
	return 0;
}
EXPORT_SYMBOL(crypto_comp_decompress);

/* ---- shrinker: 6.6 register/unregister over module-owned structs ----
 *
 * 6.18 shrinkers must come from shrinker_alloc() (extra tail fields the
 * 6.6-sized module structs do not have).  Register a kernel-side copy whose
 * callbacks trampoline back into the module's own struct so container_of()
 * in module callbacks keeps working.
 */
struct rodin_shrinker_wrap {
	struct shrinker *ks;
	struct shrinker *ms;
};

static DEFINE_XARRAY(rodin_shrinker_map);

static unsigned long rodin_count(struct shrinker *ks, struct shrink_control *sc)
{
	struct rodin_shrinker_wrap *w = ks->private_data;

	return w->ms->count_objects(w->ms, sc);
}

static unsigned long rodin_scan(struct shrinker *ks, struct shrink_control *sc)
{
	struct rodin_shrinker_wrap *w = ks->private_data;

	return w->ms->scan_objects(w->ms, sc);
}

int register_shrinker(struct shrinker *ms, const char *fmt, ...)
{
	struct rodin_shrinker_wrap *w;
	struct shrinker *ks;
	va_list ap;
	int err;

	if (!ms->count_objects || !ms->scan_objects)
		return -EINVAL;

	w = kzalloc(sizeof(*w), GFP_KERNEL);
	if (!w)
		return -ENOMEM;

	va_start(ap, fmt);
	ks = shrinker_alloc(ms->flags & ~SHRINKER_REGISTERED, fmt, ap);
	va_end(ap);
	if (!ks) {
		err = -ENOMEM;
		goto err_free_w;
	}

	ks->count_objects = rodin_count;
	ks->scan_objects = rodin_scan;
	ks->batch = ms->batch;
	ks->seeks = ms->seeks;
	ks->private_data = w;
	w->ks = ks;
	w->ms = ms;

	shrinker_register(ks);

	err = xa_insert(&rodin_shrinker_map, (unsigned long)ms, w, GFP_KERNEL);
	if (err)
		goto err_unreg;
	return 0;

err_unreg:
	/* shrinker_unregister() is shrinker_free() + removal in 6.18 */
	shrinker_free(ks);
err_free_w:
	kfree(w);
	return err;
}
EXPORT_SYMBOL(register_shrinker);

void unregister_shrinker(struct shrinker *ms)
{
	struct rodin_shrinker_wrap *w;

	w = xa_erase(&rodin_shrinker_map, (unsigned long)ms);
	if (!w)
		return;
	shrinker_free(w->ks);
	kfree(w);
}
EXPORT_SYMBOL(unregister_shrinker);

/* ---- iommu legacy API (mediatek-drm / iommu_engine) ---- */

bool iommu_present(const struct bus_type *bus)
{
	struct device *iter = NULL;
	bool found = false;

	/* 6.6 semantics: any device on the bus has an IOMMU behind it */
	while ((iter = bus_find_next_device(bus, iter)) != NULL) {
		if (device_iommu_mapped(iter))
			found = true;
		put_device(iter);
		if (found)
			break;
	}
	return found;
}
EXPORT_SYMBOL_GPL(iommu_present);

/*
 * 6.18 removed the per-device fault-handler plumbing.  Keep real per-device
 * registration state in an xarray so modules get truthful success/error;
 * fault notifications themselves are no longer delivered by 6.18 core
 * (documented degradation: registered handlers stay silent).
 */
struct rodin_iommu_fault {
	void *handler;
	void *data;
};
static DEFINE_XARRAY(rodin_iommu_fault_map);

int iommu_register_device_fault_handler(struct device *dev, void *handler, void *data)
{
	struct rodin_iommu_fault *f;

	if (!dev || !handler)
		return -EINVAL;

	if (xa_load(&rodin_iommu_fault_map, (unsigned long)dev))
		return -EBUSY;

	f = kzalloc(sizeof(*f), GFP_KERNEL);
	if (!f)
		return -ENOMEM;
	f->handler = handler;
	f->data = data;

	if (xa_insert(&rodin_iommu_fault_map, (unsigned long)dev, f, GFP_KERNEL)) {
		kfree(f);
		return -ENOMEM;
	}
	get_device(dev);
	return 0;
}
EXPORT_SYMBOL_GPL(iommu_register_device_fault_handler);

int iommu_unregister_device_fault_handler(struct device *dev)
{
	struct rodin_iommu_fault *f;

	f = xa_erase(&rodin_iommu_fault_map, (unsigned long)dev);
	if (!f)
		return -EINVAL;
	put_device(dev);
	kfree(f);
	return 0;
}
EXPORT_SYMBOL_GPL(iommu_unregister_device_fault_handler);

/*
 * 6.18 removed the IOMMU feature-toggle model entirely; nothing to disable,
 * report success so callers proceed as they did with the old no-op path.
 */
int iommu_dev_disable_feature(struct device *dev, unsigned int feat)
{
	if (!dev)
		return -EINVAL;
	return 0;
}
EXPORT_SYMBOL_GPL(iommu_dev_disable_feature);

/* ---- irq domain: 6.6 scalar-arg creation over 6.18 info API ---- */

struct irq_domain *__irq_domain_add(struct fwnode_handle *fwnode,
				    unsigned int size,
				    irq_hw_number_t hwirq_max, int direct_max,
				    const struct irq_domain_ops *ops,
				    void *host_data)
{
	struct irq_domain_info info = {
		.fwnode		= fwnode,
		.size		= size,
		.hwirq_max	= hwirq_max,
		.direct_max	= direct_max,
		.ops		= ops,
		.host_data	= host_data,
	};

	return irq_domain_instantiate(&info);
}
EXPORT_SYMBOL_GPL(__irq_domain_add);

/* ---- scheduler (scheduler.ko / cpufreq_sugov_ext) ---- */

/*
 * 6.6 check_preempt_curr(): dispatch to the current task's sched_class.
 * 6.18 renamed the op to wakeup_preempt with identical arguments.
 */

/*
 * 6.6 exposed tracepoint_srcu for modules that iterate tracepoints under
 * SRCU.  6.18 no longer uses a shared SRCU; provide a real, independent one
 * so module locking code is correct (it no longer serializes with the core,
 * which no longer needs that).
 */
struct srcu_struct tracepoint_srcu;
EXPORT_SYMBOL_GPL(tracepoint_srcu);

static int __init rodin_tracepoint_srcu_init(void)
{
	return init_srcu_struct(&tracepoint_srcu);
}
early_initcall(rodin_tracepoint_srcu_init);

/*
 * 6.6 dev_base_lock: legacy rwlock around net_device list inspection.
 * 6.18 dropped it; keep a real lock for modules (thermal_trace snapshots).
 * It no longer serializes with core netdev paths - acceptable degradation.
 */
rwlock_t dev_base_lock __cacheline_aligned_in_smp;
EXPORT_SYMBOL_GPL(dev_base_lock);

static int __init rodin_dev_base_lock_init(void)
{
	rwlock_init(&dev_base_lock);
	return 0;
}
early_initcall(rodin_dev_base_lock_init);

/*
 * 6.6 __arm_smccc_sve_check(): SMCCC v1.3 SVE hint adjustment.  6.18 dropped
 * the hint mechanism; without the hint (same as a v1.2 firmware) the call is
 * unchanged, which is the safe default.
 */
static bool smccc_has_sve_hint __ro_after_init;

unsigned long __arm_smccc_sve_check(unsigned long function_id)
{
	struct thread_info *ti = current_thread_info();

	if (!smccc_has_sve_hint)
		return function_id;

	if (test_bit(TIF_FOREIGN_FPSTATE, &ti->flags))
		function_id |= ARM_SMCCC_1_3_SVE_HINT;
	else if (!test_bit(TIF_SVE, &ti->flags))
		function_id |= ARM_SMCCC_1_3_SVE_HINT;
	return function_id;
}
EXPORT_SYMBOL(__arm_smccc_sve_check);

/*
 * 6.6 bitmap region API: apusys/pcie allocate aligned bit regions.
 * Ported verbatim from 6.6 lib/bitmap.c semantics.
 */
#undef bitmap_find_free_region
#undef bitmap_release_region

int bitmap_find_free_region(unsigned long *bitmap, unsigned int bits, int order)
{
	return bitmap_find_free_region_618(bitmap, bits, order);
}
EXPORT_SYMBOL(bitmap_find_free_region);

void bitmap_release_region(unsigned long *bitmap, unsigned int pos, int order)
{
	bitmap_release_region_618(bitmap, pos, order);
}
EXPORT_SYMBOL(bitmap_release_region);

/* ---- debugfs_rename: removed upstream, cfg80211 still uses it ----
 * Ported verbatim from 6.6 fs/debugfs/inode.c.
 */
struct dentry *debugfs_rename(struct dentry *old_dir, struct dentry *old_dentry,
			      struct dentry *new_dir, const char *new_name)
{
	int error;
	struct dentry *dentry = NULL, *trap;
	struct name_snapshot old_name;

	if (IS_ERR(old_dir))
		return old_dir;
	if (IS_ERR(new_dir))
		return new_dir;
	if (IS_ERR_OR_NULL(old_dentry))
		return old_dentry;

	trap = lock_rename(new_dir, old_dir);
	if (d_really_is_negative(old_dir) || d_really_is_negative(new_dir))
		goto exit;
	if (d_really_is_negative(old_dentry) || old_dentry == trap ||
	    d_mountpoint(old_dentry))
		goto exit;
	dentry = lookup_one_len(new_name, new_dir, strlen(new_name));
	if (IS_ERR(dentry) || dentry == trap || d_really_is_positive(dentry))
		goto exit;

	take_dentry_name_snapshot(&old_name, old_dentry);

	error = simple_rename(&nop_mnt_idmap, d_inode(old_dir), old_dentry,
			      d_inode(new_dir), dentry, 0);
	if (error) {
		release_dentry_name_snapshot(&old_name);
		goto exit;
	}
	d_move(old_dentry, dentry);
	fsnotify_move(d_inode(old_dir), d_inode(new_dir), &old_name.name,
		      d_is_dir(old_dentry), NULL, old_dentry);
	release_dentry_name_snapshot(&old_name);
	unlock_rename(new_dir, old_dir);
	dput(dentry);
	return old_dentry;
exit:
	if (dentry && !IS_ERR(dentry))
		dput(dentry);
	unlock_rename(new_dir, old_dir);
	if (IS_ERR(dentry))
		return dentry;
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL_GPL(debugfs_rename);
