// SPDX-License-Identifier: GPL-2.0
/*
 * lkfb - debug-capture v2: minimal early framebuffer on MTK lk logo buffer
 * (reserved-memory/mblock-15-framebuffer, compatible "mediatek,framebuffer").
 * Registers a linear XRGB8888 fbdev so fbcon prints kernel log on screen
 * from postcore_initcall (~0.1s), long before any display module loads.
 * Purpose: photograph last lines when boot hangs and pstore is lost to power-off.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

static struct fb_info *lkfb_info;

static int lkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (var->xres != info->var.xres || var->yres != info->var.yres ||
	    var->bits_per_pixel != info->var.bits_per_pixel)
		return -EINVAL;
	return 0;
}

static int lkfb_set_par(struct fb_info *info) { return 0; }

static int lkfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			  unsigned blue, unsigned transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;

	if (regno >= 16)
		return 1;
	pal[regno] = ((red & 0xff00) << 8) | (green & 0xff00) |
		     ((blue & 0xff00) >> 8);
	return 0;
}

static int lkfb_blank(int blank_mode, struct fb_info *info)
{
	/* never blank: keep console visible for hang capture */
	return 0;
}

static const struct fb_ops lkfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= lkfb_check_var,
	.fb_set_par	= lkfb_set_par,
	.fb_setcolreg	= lkfb_setcolreg,
	.fb_blank	= lkfb_blank,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static int __init lkfb_init(void)
{
	struct device_node *np;
	struct resource res;
	struct fb_info *info;
	u32 w = 1220, h = 2712, bpp = 32;
	u32 line = (w * bpp) / 8;
	u32 vlines;
	int ret;

	np = of_find_node_by_path("/reserved-memory/mblock-15-framebuffer");
	if (!np)
		return -ENODEV;
	if (of_address_to_resource(np, 0, &res))
		return -ENXIO;
	of_node_put(np);

	if (resource_size(&res) < line * h) {
		pr_err("lkfb: buffer too small\n");
		return -ENOMEM;
	}

	info = framebuffer_alloc(sizeof(u32) * 16, NULL);
	if (!info)
		return -ENOMEM;

	/*
	 * mblock-15-framebuffer is "map non-reusable" reserved RAM, so
	 * MEMREMAP_WC would trip memremap's "attempted on ram" WARN_ONCE
	 * and return NULL (framebuffer never registers). MEMREMAP_WB
	 * resolves to the direct map, which is coherent with the display
	 * engine's reads, and succeeds without warning.
	 */
	info->screen_base = memremap(res.start, resource_size(&res), MEMREMAP_WB);
	if (!info->screen_base) {
		framebuffer_release(info);
		return -ENOMEM;
	}

	strscpy(info->fix.id, "lkfb", sizeof(info->fix.id));
	info->fix.smem_start = res.start;
	info->fix.smem_len = resource_size(&res);
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.line_length = line;
	info->fix.accel = FB_ACCEL_NONE;

	info->var.xres = w;
	info->var.yres = h;
	vlines = min_t(u32, resource_size(&res) / line, h * 4);
	info->var.xres_virtual = w;
	info->var.yres_virtual = vlines;
	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.bits_per_pixel = bpp;
	info->var.red.offset = 16;   info->var.red.length = 8;
	info->var.green.offset = 8;  info->var.green.length = 8;
	info->var.blue.offset = 0;   info->var.blue.length = 8;
	info->var.transp.offset = 24; info->var.transp.length = 8;
	info->var.activate = FB_ACTIVATE_NOW;
	info->var.vmode = FB_VMODE_NONINTERLACED;
	info->var.height = -1;
	info->var.width = -1;

	info->fbops = &lkfb_ops;
	info->flags = 0; /* FBINFO_FLAG_DEFAULT removed in .142 */
	info->pseudo_palette = info->par;

	ret = fb_alloc_cmap(&info->cmap, 16, 0);
	if (ret) {
		memunmap(info->screen_base);
		framebuffer_release(info);
		return ret;
	}

	ret = register_framebuffer(info);
	if (ret) {
		fb_dealloc_cmap(&info->cmap);
		memunmap(info->screen_base);
		framebuffer_release(info);
		return ret;
	}

	lkfb_info = info;
	pr_info("lkfb: %ux%u XRGB8888 @ %pa..%pa - kernel log on screen from now\n",
		w, h, &res.start, &res.end);
	return 0;
}
postcore_initcall(lkfb_init);
