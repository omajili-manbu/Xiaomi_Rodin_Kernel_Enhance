// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6-compat: PM/thermal/misc-driver symbols 6.18 renamed or removed,
 * for prebuilt 6.6 vendor modules (mediatek-drm, charger, touch, leds...).
 */
#define fwnode_handle_put		fwnode_handle_put_618
#define device_get_child_node_count	device_get_child_node_count_618
#define dma_sync_sg_for_cpu		dma_sync_sg_for_cpu_618
#define dma_sync_sg_for_device		dma_sync_sg_for_device_618
#define dma_sync_single_for_cpu		dma_sync_single_for_cpu_618
#define dma_sync_single_for_device	dma_sync_single_for_device_618
#define dma_pool_create			dma_pool_create_618
#define tty_port_tty_hangup		tty_port_tty_hangup_618
#define pwmchip_add			pwmchip_add_618
#define page_pool_put_page		page_pool_put_page_618

#include <linux/module.h>
#include <linux/property.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/tty.h>
#include <linux/tty_port.h>
#include <linux/pwm.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <net/page_pool/helpers.h>
#include <linux/thermal.h>

/* 6.18 turned these names into macros; the 6.6 symbols come back below */
#ifdef fwnode_handle_put
#undef fwnode_handle_put
#endif
#ifdef device_get_child_node_count
#undef device_get_child_node_count
#endif
#ifdef dma_sync_sg_for_cpu
#undef dma_sync_sg_for_cpu
#endif
#ifdef dma_sync_sg_for_device
#undef dma_sync_sg_for_device
#endif
#ifdef dma_sync_single_for_cpu
#undef dma_sync_single_for_cpu
#endif
#ifdef dma_sync_single_for_device
#undef dma_sync_single_for_device
#endif
#ifdef dma_pool_create
#undef dma_pool_create
#endif
#ifdef tty_port_tty_hangup
#undef tty_port_tty_hangup
#endif
#ifdef pwmchip_add
#undef pwmchip_add
#endif
#ifdef pwm_apply_state
#undef pwm_apply_state
#endif
#ifdef page_pool_put_defragged_page
#undef page_pool_put_page
#undef page_pool_put_defragged_page
#endif
#ifdef power_supply_get_by_phandle
#undef power_supply_get_by_phandle
#endif
#ifdef devm_power_supply_get_by_phandle
#undef devm_power_supply_get_by_phandle
#endif
#ifdef devm_gpio_request
#undef devm_gpio_request
#endif
#ifdef snd_soc_unregister_component
#undef snd_soc_unregister_component
#endif
#ifdef dapm_kcontrol_get_value
#undef dapm_kcontrol_get_value
#endif
#ifdef snd_soc_dapm_kcontrol_widget
#undef snd_soc_dapm_kcontrol_widget
#endif
#ifdef dapm_clock_event
#undef dapm_clock_event
#endif
#ifdef dapm_regulator_event
#undef dapm_regulator_event
#endif
#ifdef v4l2_async_register_subdev
#undef v4l2_async_register_subdev
#endif
#ifdef v4l2_device_register_subdev
#undef v4l2_device_register_subdev
#endif
#ifdef __v4l2_find_nearest_size
#undef __v4l2_find_nearest_size
#endif
#ifdef ___drm_dbg
#undef ___drm_dbg
#endif
#ifdef sdio_register_driver
#undef sdio_register_driver
#endif
#ifdef register_virtio_driver
#undef register_virtio_driver
#endif


#undef fwnode_handle_put
#undef device_get_child_node_count
#undef dma_sync_sg_for_cpu
#undef dma_sync_sg_for_device
#undef dma_sync_single_for_cpu
#undef dma_sync_single_for_device
#undef dma_pool_create
#undef tty_port_tty_hangup
#undef pwmchip_add
#undef page_pool_put_defragged_page

/* ---- property helpers (6.18 made them inline) ---- */

void fwnode_handle_put(struct fwnode_handle *fwnode)
{
	fwnode_handle_put_618(fwnode);
}
EXPORT_SYMBOL_GPL(fwnode_handle_put);

unsigned int device_get_child_node_count(const struct device *dev)
{
	return device_get_child_node_count_618(dev);
}
EXPORT_SYMBOL_GPL(device_get_child_node_count);

/* ---- dma sync helpers (6.18 inline) ---- */

void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sgl,
			 int nents, enum dma_data_direction dir)
{
	dma_sync_sg_for_cpu_618(dev, sgl, nents, dir);
}
EXPORT_SYMBOL(dma_sync_sg_for_cpu);

void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sgl,
			    int nents, enum dma_data_direction dir)
{
	dma_sync_sg_for_device_618(dev, sgl, nents, dir);
}
EXPORT_SYMBOL(dma_sync_sg_for_device);

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t addr, size_t size,
			     enum dma_data_direction dir)
{
	dma_sync_single_for_cpu_618(dev, addr, size, dir);
}
EXPORT_SYMBOL(dma_sync_single_for_cpu);

void dma_sync_single_for_device(struct device *dev, dma_addr_t addr, size_t size,
				enum dma_data_direction dir)
{
	dma_sync_single_for_device_618(dev, addr, size, dir);
}
EXPORT_SYMBOL(dma_sync_single_for_device);

struct dma_pool *dma_pool_create(const char *name, struct device *dev,
				 size_t size, size_t align, size_t boundary)
{
	return dma_pool_create_618(name, dev, size, align, boundary);
}
EXPORT_SYMBOL(dma_pool_create);

/* ---- tty ---- */

void tty_port_tty_hangup(struct tty_port *port, bool check_clocal)
{
	tty_port_tty_hangup_618(port, check_clocal);
}
EXPORT_SYMBOL_GPL(tty_port_tty_hangup);

/* ---- pwm ---- */

int pwmchip_add(struct pwm_chip *chip)
{
	return pwmchip_add_618(chip);
}
EXPORT_SYMBOL_GPL(pwmchip_add);

int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state);
int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state)
	__alias(pwm_apply_might_sleep);
EXPORT_SYMBOL_GPL(pwm_apply_state);

/* ---- page_pool (ccci_dpmaif / wlan_page_pool) ---- */

/* 6.6 page_pool_put_defragged_page(pool, page, dma_sync_size, allow_direct) */
void page_pool_put_defragged_page(struct page_pool *pool, struct page *page,
				  unsigned int dma_sync_size,
				  bool allow_direct)
{
	page_pool_put_page_618(pool, page, dma_sync_size, allow_direct);
}
EXPORT_SYMBOL(page_pool_put_defragged_page);

/* ---- power supply: 6.6 phandle lookups (charger family) ---- */

extern struct class *power_supply_class;

static int power_supply_match_device_node(struct device *dev, const void *data)
{
	return dev->parent && dev->parent->of_node == data;
}

struct power_supply *power_supply_get_by_phandle(struct device_node *np,
							const char *property)
{
	struct device_node *power_supply_np;
	struct power_supply *psy = NULL;
	struct device *dev;

	power_supply_np = of_parse_phandle(np, property, 0);
	if (!power_supply_np)
		return ERR_PTR(-ENODEV);

	dev = class_find_device(power_supply_class, NULL, power_supply_np,
				power_supply_match_device_node);
	of_node_put(power_supply_np);

	if (dev) {
		psy = dev_get_drvdata(dev);
		atomic_inc(&psy->use_cnt);
	}

	return psy;
}
EXPORT_SYMBOL_GPL(power_supply_get_by_phandle);

static void devm_power_supply_put(void *res)
{
	struct power_supply *psy = *(struct power_supply **)res;

	power_supply_put(psy);
}

struct power_supply *devm_power_supply_get_by_phandle(struct device *dev,
						      const char *property)
{
	struct power_supply **ptr, *psy;

	if (!dev->of_node)
		return ERR_PTR(-ENODEV);

	ptr = devres_alloc(devm_power_supply_put, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	psy = power_supply_get_by_phandle(dev->of_node, property);
	if (IS_ERR_OR_NULL(psy)) {
		devres_free(ptr);
	} else {
		*ptr = psy;
		devres_add(dev, ptr);
	}
	return psy;
}
EXPORT_SYMBOL_GPL(devm_power_supply_get_by_phandle);

/* ---- gpio legacy devm (xiaomi_wifi_gpio) ---- */

int devm_gpio_request(struct device *dev, unsigned gpio, const char *label)
{
	return devm_gpio_request_one(dev, gpio, 0, label);
}
EXPORT_SYMBOL_GPL(devm_gpio_request);

/* ---- ASoC: 6.6 entry points over 6.18 internals ---- */

void snd_soc_unregister_component(struct device *dev)
{
	snd_soc_unregister_component_by_driver(dev, NULL);
}
EXPORT_SYMBOL_GPL(snd_soc_unregister_component);

unsigned int dapm_kcontrol_get_value(const struct snd_kcontrol *kcontrol)
{
	return snd_soc_dapm_kcontrol_get_value(kcontrol);
}
EXPORT_SYMBOL_GPL(dapm_kcontrol_get_value);

struct snd_soc_dapm_widget *snd_soc_dapm_kcontrol_widget(
	const struct snd_kcontrol *kcontrol)
{
	return snd_soc_dapm_kcontrol_to_widget(kcontrol);
}
EXPORT_SYMBOL_GPL(snd_soc_dapm_kcontrol_widget);

int dapm_clock_event(struct snd_soc_dapm_widget *w,
		     struct snd_kcontrol *kcontrol, int event)
{
	return snd_soc_dapm_clock_event(w, kcontrol, event);
}
EXPORT_SYMBOL_GPL(dapm_clock_event);

int dapm_regulator_event(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol, int event)
{
	return snd_soc_dapm_regulator_event(w, kcontrol, event);
}
EXPORT_SYMBOL_GPL(dapm_regulator_event);

/* ---- v4l2: 6.6 entry points over 6.18 module-aware internals ---- */

int v4l2_async_register_subdev(struct v4l2_subdev *sd)
{
	return __v4l2_async_register_subdev(sd, sd->owner);
}
EXPORT_SYMBOL(v4l2_async_register_subdev);

int v4l2_device_register_subdev(struct v4l2_device *v4l2_dev,
				struct v4l2_subdev *sd)
{
	return __v4l2_device_register_subdev(v4l2_dev, sd, sd->owner);
}
EXPORT_SYMBOL(v4l2_device_register_subdev);

const void *__v4l2_find_nearest_size(const void *array, size_t array_size,
				     size_t entry_size, size_t width_offset,
				     size_t height_offset, int width, int height)
{
	return __v4l2_find_nearest_size_conditional(array, array_size, entry_size,
						    width_offset, height_offset,
						    NULL, width, height);
}
EXPORT_SYMBOL_GPL(__v4l2_find_nearest_size);

/* ---- drm: 6.6 print entry points ---- */

void ___drm_dbg(const struct drm_device *dev, enum drm_debug_category category,
		const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);
	vaf.fmt = format;
	vaf.va = &args;
	__drm_dev_dbg(NULL, dev ? dev->dev : NULL, category, "%pV", &vaf);
	va_end(args);
}
EXPORT_SYMBOL(___drm_dbg);

/* ---- sdio / virtio: 6.6 owner-less registration forms ---- */

int sdio_register_driver(struct sdio_driver *drv)
{
	return __sdio_register_driver(drv, THIS_MODULE);
}
EXPORT_SYMBOL_GPL(sdio_register_driver);

int register_virtio_driver(struct virtio_driver *driver)
{
	return __register_virtio_driver(driver, THIS_MODULE);
}
EXPORT_SYMBOL_GPL(register_virtio_driver);
