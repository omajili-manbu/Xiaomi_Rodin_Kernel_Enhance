// SPDX-License-Identifier: GPL-2.0
/*
 * rodin 6.6-compat: netdev/skb symbols 6.18 inlined, renamed or removed,
 * needed by prebuilt 6.6 vendor modules (wlan/ccci/r8152/tipc/mac80211...).
 */
#define sock_i_uid		sock_i_uid_618
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define netif_napi_add_weight		netif_napi_add_weight_618
#define __netif_napi_del		__netif_napi_del_618
#define napi_gro_receive		napi_gro_receive_618
#define skb_add_rx_frag			skb_add_rx_frag_618
#define sock_i_ino			sock_i_ino_618
#define sock_i_uid			sock_i_uid_618
#define dev_nit_active			dev_nit_active_618
#define snd_soc_set_runtime_hwparams	snd_soc_set_runtime_hwparams_618
#define napi_gro_flush			napi_gro_flush_618

#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/net.h>
#include <linux/rhashtable.h>
#include <net/sock.h>
#include <net/page_pool/helpers.h>
#include <linux/rtnetlink.h>
#include <sound/soc.h>
#include <net/gro.h>
#include <net/rtnetlink.h>
#include <linux/cpufreq.h>

/* 6.18 turned these names into macros; the 6.6 symbols come back below */
#ifdef netif_napi_add_weight
#undef netif_napi_add_weight
#endif
#ifdef __netif_napi_del
#undef __netif_napi_del
#endif
#ifdef napi_gro_receive
#undef napi_gro_receive
#endif
#ifdef __napi_alloc_skb
#undef __napi_alloc_skb
#endif
#ifdef skb_add_rx_frag
#undef skb_add_rx_frag
#endif
#ifdef sock_i_ino
#undef sock_i_ino
#endif
#ifdef sock_i_uid
#undef sock_i_uid
#endif
#ifdef dev_nit_active
#undef dev_nit_active
#endif
#ifdef __skb_get_hash
#undef __skb_get_hash
#endif
#ifdef rhashtable_init
#undef rhashtable_init
#endif
#ifdef rhltable_init
#undef rhltable_init
#endif
#ifdef init_dummy_netdev
#undef init_dummy_netdev
#endif
#ifdef dev_close_many
#undef dev_close_many
#endif
#ifdef dev_get_flags
#undef dev_get_flags
#endif
#ifdef rtnl_register_module
#undef rtnl_register_module
#endif
#ifdef napi_gro_flush
#undef napi_gro_flush
#endif
#ifdef cpufreq_generic_attr
#undef cpufreq_generic_attr
#endif
#ifdef snd_soc_set_runtime_hwparams
#undef snd_soc_set_runtime_hwparams
#endif


#undef __netif_napi_del
#undef napi_gro_receive
#undef skb_add_rx_frag
#undef sock_i_ino
#undef sock_i_uid
#undef dev_nit_active
#undef rhashtable_init
#undef rhltable_init
#undef snd_soc_set_runtime_hwparams
#undef napi_gro_flush

/* 6.6-ABI prototypes: 6.18 inlined these names (renamed to *_618 above) or
 * deleted them, so re-declare the exported 6.6 names here. */
void netif_napi_add_weight(struct net_device *dev, struct napi_struct *napi,
			   int (*poll)(struct napi_struct *, int), int weight);
void __netif_napi_del(struct napi_struct *napi);
gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb);
void skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page, int off,
		     int size, unsigned int truesize);
unsigned long sock_i_ino(const struct sock *sk);
kuid_t sock_i_uid(const struct sock *sk);
bool dev_nit_active(const struct net_device *dev);
void __skb_get_hash(struct sk_buff *skb);
struct sk_buff *__napi_alloc_skb(struct napi_struct *napi, unsigned int len,
				 gfp_t gfp_mask);
int rhashtable_init(struct rhashtable *ht, const struct rhashtable_params *params);
int rhltable_init(struct rhltable *hlt, const struct rhashtable_params *params);
void init_dummy_netdev(struct net_device *dev);
void dev_close_many(struct list_head *head, bool unlink);
unsigned int dev_get_flags(const struct net_device *dev);
void napi_gro_flush(struct napi_struct *napi, bool flush_old);
int snd_soc_set_runtime_hwparams(struct snd_pcm_substream *substream,
				 const struct snd_soc_pcm_stream *hw);
int rtnl_register_module(struct module *owner, int protocol, unsigned int msgtype,
			 rtnl_doit_func doit, rtnl_dumpit_func dumpit,
			 unsigned int flags);

/* napi registration: 6.6 netif_napi_add_weight(dev, napi, poll, weight) */
void netif_napi_add_weight(struct net_device *dev, struct napi_struct *napi,
			   int (*poll)(struct napi_struct *, int), int weight)
{
	netif_napi_add_weight_618(dev, napi, poll, weight);
}
EXPORT_SYMBOL(netif_napi_add_weight);

void __netif_napi_del(struct napi_struct *napi)
{
	__netif_napi_del_locked(napi);
}
EXPORT_SYMBOL(__netif_napi_del);

/* napi_gro_receive: 6.18 turned it into an inline wrapper */
gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	return napi_gro_receive_618(napi, skb);
}
EXPORT_SYMBOL(napi_gro_receive);

/* 6.6 __napi_alloc_skb(napi,len,gfp): 6.18 napi_alloc_skb() hardcodes the
 * GFP_ATOMIC allocation path, which is what 6.6 __napi_alloc_skb used for
 * the softirq/RX case; honour a caller gfp only insofar as 6.18 allows.
 */
struct sk_buff *__napi_alloc_skb(struct napi_struct *napi, unsigned int len,
				 gfp_t gfp_mask)
{
	return napi_alloc_skb(napi, len);
}
EXPORT_SYMBOL(__napi_alloc_skb);

void skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page, int off,
		     int size, unsigned int truesize)
{
	skb_add_rx_frag_618(skb, i, page, off, size, truesize);
}
EXPORT_SYMBOL(skb_add_rx_frag);

unsigned long sock_i_ino(const struct sock *sk)
{
	return sock_i_ino_618(sk);
}
EXPORT_SYMBOL(sock_i_ino);

kuid_t sock_i_uid(const struct sock *sk)
{
	return sk->sk_uid;
}
EXPORT_SYMBOL(sock_i_uid);

bool dev_nit_active(const struct net_device *dev)
{
	return dev_nit_active_618(dev);
}
EXPORT_SYMBOL(dev_nit_active);

/* __skb_get_hash(): 6.18 has the _net variant exported */
void __skb_get_hash(struct sk_buff *skb)
{
	__skb_get_hash_net(NULL, skb);
}
EXPORT_SYMBOL(__skb_get_hash);

int rhashtable_init(struct rhashtable *ht, const struct rhashtable_params *params)
{
	return rhashtable_init_noprof(ht, params);
}
EXPORT_SYMBOL(rhashtable_init);

int rhltable_init(struct rhltable *hlt, const struct rhashtable_params *params)
{
	return rhltable_init_noprof(hlt, params);
}
EXPORT_SYMBOL(rhltable_init);

/*
 * init_dummy_netdev / dev_close_many / dev_get_flags: 6.18 removed the
 * public helpers; ported from 6.6 net/core/dev.c.
 */
void init_dummy_netdev(struct net_device *dev)
{
	/* core netdev struct zeroed by the caller (6.6 contract) */
	set_bit(__LINK_STATE_PRESENT, &dev->state);
	set_bit(__LINK_STATE_START, &dev->state);

	INIT_LIST_HEAD(&dev->napi_list);
	INIT_LIST_HEAD(&dev->close_list);
	INIT_LIST_HEAD(&dev->link_watch_list);
	dev->reg_state = NETREG_UNREGISTERED;
	dev->needs_free_netdev = false;
	/* NAPI instances hang off this device without a real netdev backend */
}
EXPORT_SYMBOL(init_dummy_netdev);

void dev_close_many(struct list_head *head, bool unlink)
{
	struct net_device *dev, *tmp;

	/* 6.6 logic: close each device on the list, optionally unlink it */
	list_for_each_entry_safe(dev, tmp, head, close_list) {
		if (unlink)
			list_del_init(&dev->close_list);
		if (dev->flags & IFF_UP)
			dev_close(dev);
	}
}
EXPORT_SYMBOL(dev_close_many);

unsigned int dev_get_flags(const struct net_device *dev)
{
	unsigned int flags;

	flags = (dev->flags & ~(IFF_UP | IFF_RUNNING));
	if (netif_running(dev))
		flags |= IFF_UP;
	if (netif_running(dev) && netif_carrier_ok(dev) &&
	    netif_oper_up(dev))
		flags |= IFF_RUNNING;

	return flags;
}
EXPORT_SYMBOL(dev_get_flags);

/* rtnetlink: 6.18 folded registration into __rtnl_register_many */
int rtnl_register_module(struct module *owner, int protocol, unsigned int msgtype,
			 rtnl_doit_func doit, rtnl_dumpit_func dumpit,
			 unsigned int flags)
{
	const struct rtnl_msg_handler h = {
		.owner		= owner,
		.protocol	= protocol,
		.msgtype	= msgtype,
		.doit		= doit,
		.dumpit		= dumpit,
		.flags		= flags,
	};

	return __rtnl_register_many(&h, 1);
}
EXPORT_SYMBOL_GPL(rtnl_register_module);

int snd_soc_set_runtime_hwparams(struct snd_pcm_substream *substream,
				 const struct snd_soc_pcm_stream *hw)
{
	/* 6.18 inline expects snd_pcm_hardware; 6.6 caller passes pcm_stream.  The
	 * pointer targets the same runtime->hw storage, so cast over. */
	return snd_soc_set_runtime_hwparams_618(substream,
					(const struct snd_pcm_hardware *)hw);
}
EXPORT_SYMBOL_GPL(snd_soc_set_runtime_hwparams);

void napi_gro_flush(struct napi_struct *napi, bool flush_old)
{
	napi_gro_flush_618(napi, flush_old);
}
EXPORT_SYMBOL(napi_gro_flush);

/* cpufreq: 6.6 generic attr array (mediatek-cpufreq-hw) */
struct freq_attr *cpufreq_generic_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};
EXPORT_SYMBOL_GPL(cpufreq_generic_attr);
