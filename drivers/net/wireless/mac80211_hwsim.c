/*
 * mac80211_hwsim - software simulator of 802.11 radio(s) for mac80211
 * Copyright (c) 2008, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * TODO:
 * - IBSS mode simulation (Beacon transmission with competition for "air time")
 * - RX filtering based on filter configuration (data->rx_filter)
 */

#include <linux/list.h>
#include <linux/spinlock.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/debugfs.h>

MODULE_AUTHOR("Jouni Malinen");
MODULE_DESCRIPTION("Software simulator of 802.11 radio(s) for mac80211");
MODULE_LICENSE("GPL");

static int radios = 2;
module_param(radios, int, 0444);
MODULE_PARM_DESC(radios, "Number of simulated radios");

/**
 * enum hwsim_regtest - the type of regulatory tests we offer
 *
 * These are the different values you can use for the regtest
 * module parameter. This is useful to help test world roaming
 * and the driver regulatory_hint() call and combinations of these.
 * If you want to do specific alpha2 regulatory domain tests simply
 * use the userspace regulatory request as that will be respected as
 * well without the need of this module parameter. This is designed
 * only for testing the driver regulatory request, world roaming
 * and all possible combinations.
 *
 * @HWSIM_REGTEST_DISABLED: No regulatory tests are performed,
 * 	this is the default value.
 * @HWSIM_REGTEST_DRIVER_REG_FOLLOW: Used for testing the driver regulatory
 *	hint, only one driver regulatory hint will be sent as such the
 * 	secondary radios are expected to follow.
 * @HWSIM_REGTEST_DRIVER_REG_ALL: Used for testing the driver regulatory
 * 	request with all radios reporting the same regulatory domain.
 * @HWSIM_REGTEST_DIFF_COUNTRY: Used for testing the drivers calling
 * 	different regulatory domains requests. Expected behaviour is for
 * 	an intersection to occur but each device will still use their
 * 	respective regulatory requested domains. Subsequent radios will
 * 	use the resulting intersection.
 * @HWSIM_REGTEST_WORLD_ROAM: Used for testing the world roaming. We acomplish
 *	this by using a custom beacon-capable regulatory domain for the first
 *	radio. All other device world roam.
 * @HWSIM_REGTEST_CUSTOM_WORLD: Used for testing the custom world regulatory
 * 	domain requests. All radios will adhere to this custom world regulatory
 * 	domain.
 * @HWSIM_REGTEST_CUSTOM_WORLD_2: Used for testing 2 custom world regulatory
 * 	domain requests. The first radio will adhere to the first custom world
 * 	regulatory domain, the second one to the second custom world regulatory
 * 	domain. All other devices will world roam.
 * @HWSIM_REGTEST_STRICT_FOLLOW_: Used for testing strict regulatory domain
 *	settings, only the first radio will send a regulatory domain request
 *	and use strict settings. The rest of the radios are expected to follow.
 * @HWSIM_REGTEST_STRICT_ALL: Used for testing strict regulatory domain
 *	settings. All radios will adhere to this.
 * @HWSIM_REGTEST_STRICT_AND_DRIVER_REG: Used for testing strict regulatory
 *	domain settings, combined with secondary driver regulatory domain
 *	settings. The first radio will get a strict regulatory domain setting
 *	using the first driver regulatory request and the second radio will use
 *	non-strict settings using the second driver regulatory request. All
 *	other devices should follow the intersection created between the
 *	first two.
 * @HWSIM_REGTEST_ALL: Used for testing every possible mix. You will need
 * 	at least 6 radios for a complete test. We will test in this order:
 * 	1 - driver custom world regulatory domain
 * 	2 - second custom world regulatory domain
 * 	3 - first driver regulatory domain request
 * 	4 - second driver regulatory domain request
 * 	5 - strict regulatory domain settings using the third driver regulatory
 * 	    domain request
 * 	6 and on - should follow the intersection of the 3rd, 4rth and 5th radio
 * 	           regulatory requests.
 */
enum hwsim_regtest {
	HWSIM_REGTEST_DISABLED = 0,
	HWSIM_REGTEST_DRIVER_REG_FOLLOW = 1,
	HWSIM_REGTEST_DRIVER_REG_ALL = 2,
	HWSIM_REGTEST_DIFF_COUNTRY = 3,
	HWSIM_REGTEST_WORLD_ROAM = 4,
	HWSIM_REGTEST_CUSTOM_WORLD = 5,
	HWSIM_REGTEST_CUSTOM_WORLD_2 = 6,
	HWSIM_REGTEST_STRICT_FOLLOW = 7,
	HWSIM_REGTEST_STRICT_ALL = 8,
	HWSIM_REGTEST_STRICT_AND_DRIVER_REG = 9,
	HWSIM_REGTEST_ALL = 10,
};

/* Set to one of the HWSIM_REGTEST_* values above */
static int regtest = HWSIM_REGTEST_DISABLED;
module_param(regtest, int, 0444);
MODULE_PARM_DESC(regtest, "The type of regulatory test we want to run");

static const char *hwsim_alpha2s[] = {
	"FI",
	"AL",
	"US",
	"DE",
	"JP",
	"AL",
};

static const struct ieee80211_regdomain hwsim_world_regdom_custom_01 = {
	.n_reg_rules = 4,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(2412-10, 2462+10, 40, 0, 20, 0),
		REG_RULE(2484-10, 2484+10, 40, 0, 20, 0),
		REG_RULE(5150-10, 5240+10, 40, 0, 30, 0),
		REG_RULE(5745-10, 5825+10, 40, 0, 30, 0),
	}
};

static const struct ieee80211_regdomain hwsim_world_regdom_custom_02 = {
	.n_reg_rules = 2,
	.alpha2 =  "99",
	.reg_rules = {
		REG_RULE(2412-10, 2462+10, 40, 0, 20, 0),
		REG_RULE(5725-10, 5850+10, 40, 0, 30,
			NL80211_RRF_PASSIVE_SCAN | NL80211_RRF_NO_IBSS),
	}
};

struct hwsim_vif_priv {
	u32 magic;
	u8 bssid[ETH_ALEN];
	bool assoc;
	u16 aid;
};

#define HWSIM_VIF_MAGIC	0x69537748

static inline void hwsim_check_magic(struct ieee80211_vif *vif)
{
	struct hwsim_vif_priv *vp = (void *)vif->drv_priv;
	WARN_ON(vp->magic != HWSIM_VIF_MAGIC);
}

static inline void hwsim_set_magic(struct ieee80211_vif *vif)
{
	struct hwsim_vif_priv *vp = (void *)vif->drv_priv;
	vp->magic = HWSIM_VIF_MAGIC;
}

static inline void hwsim_clear_magic(struct ieee80211_vif *vif)
{
	struct hwsim_vif_priv *vp = (void *)vif->drv_priv;
	vp->magic = 0;
}

struct hwsim_sta_priv {
	u32 magic;
};

#define HWSIM_STA_MAGIC	0x6d537748

static inline void hwsim_check_sta_magic(struct ieee80211_sta *sta)
{
	struct hwsim_sta_priv *sp = (void *)sta->drv_priv;
	WARN_ON(sp->magic != HWSIM_STA_MAGIC);
}

static inline void hwsim_set_sta_magic(struct ieee80211_sta *sta)
{
	struct hwsim_sta_priv *sp = (void *)sta->drv_priv;
	sp->magic = HWSIM_STA_MAGIC;
}

static inline void hwsim_clear_sta_magic(struct ieee80211_sta *sta)
{
	struct hwsim_sta_priv *sp = (void *)sta->drv_priv;
	sp->magic = 0;
}

static struct class *hwsim_class;

static struct net_device *hwsim_mon; /* global monitor netdev */

#define CHAN2G(_freq)  { \
	.band = IEEE80211_BAND_2GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
	.max_power = 20, \
}

#define CHAN5G(_freq) { \
	.band = IEEE80211_BAND_5GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_freq), \
	.max_power = 20, \
}

static const struct ieee80211_channel hwsim_channels_2ghz[] = {
	CHAN2G(2412), /* Channel 1 */
	CHAN2G(2417), /* Channel 2 */
	CHAN2G(2422), /* Channel 3 */
	CHAN2G(2427), /* Channel 4 */
	CHAN2G(2432), /* Channel 5 */
	CHAN2G(2437), /* Channel 6 */
	CHAN2G(2442), /* Channel 7 */
	CHAN2G(2447), /* Channel 8 */
	CHAN2G(2452), /* Channel 9 */
	CHAN2G(2457), /* Channel 10 */
	CHAN2G(2462), /* Channel 11 */
	CHAN2G(2467), /* Channel 12 */
	CHAN2G(2472), /* Channel 13 */
	CHAN2G(2484), /* Channel 14 */
};

static const struct ieee80211_channel hwsim_channels_5ghz[] = {
	CHAN5G(5180), /* Channel 36 */
	CHAN5G(5200), /* Channel 40 */
	CHAN5G(5220), /* Channel 44 */
	CHAN5G(5240), /* Channel 48 */

	CHAN5G(5260), /* Channel 52 */
	CHAN5G(5280), /* Channel 56 */
	CHAN5G(5300), /* Channel 60 */
	CHAN5G(5320), /* Channel 64 */

	CHAN5G(5500), /* Channel 100 */
	CHAN5G(5520), /* Channel 104 */
	CHAN5G(5540), /* Channel 108 */
	CHAN5G(5560), /* Channel 112 */
	CHAN5G(5580), /* Channel 116 */
	CHAN5G(5600), /* Channel 120 */
	CHAN5G(5620), /* Channel 124 */
	CHAN5G(5640), /* Channel 128 */
	CHAN5G(5660), /* Channel 132 */
	CHAN5G(5680), /* Channel 136 */
	CHAN5G(5700), /* Channel 140 */

	CHAN5G(5745), /* Channel 149 */
	CHAN5G(5765), /* Channel 153 */
	CHAN5G(5785), /* Channel 157 */
	CHAN5G(5805), /* Channel 161 */
	CHAN5G(5825), /* Channel 165 */
};

static const struct ieee80211_rate hwsim_rates[] = {
	{ .bitrate = 10 },
	{ .bitrate = 20, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110, .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 60 },
	{ .bitrate = 90 },
	{ .bitrate = 120 },
	{ .bitrate = 180 },
	{ .bitrate = 240 },
	{ .bitrate = 360 },
	{ .bitrate = 480 },
	{ .bitrate = 540 }
};

static spinlock_t hwsim_radio_lock;
static struct list_head hwsim_radios;

struct mac80211_hwsim_data {
	struct list_head list;
	struct ieee80211_hw *hw;
	struct device *dev;
	struct ieee80211_supported_band bands[2];
	struct ieee80211_channel channels_2ghz[ARRAY_SIZE(hwsim_channels_2ghz)];
	struct ieee80211_channel channels_5ghz[ARRAY_SIZE(hwsim_channels_5ghz)];
	struct ieee80211_rate rates[ARRAY_SIZE(hwsim_rates)];

	struct ieee80211_channel *channel;
	unsigned long beacon_int; /* in jiffies unit */
	unsigned int rx_filter;
	int started;
	struct timer_list beacon_timer;
	enum ps_mode {
		PS_DISABLED, PS_ENABLED, PS_AUTO_POLL, PS_MANUAL_POLL
	} ps;
	bool ps_poll_pending;
	struct dentry *debugfs;
	struct dentry *debugfs_ps;

	/*
	 * Only radios in the same group can communicate together (the
	 * channel has to match too). Each bit represents a group. A
	 * radio can be in more then one group.
	 */
	u64 group;
	struct dentry *debugfs_group;
};


struct hwsim_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	u8 rt_flags;
	u8 rt_rate;
	__le16 rt_channel;
	__le16 rt_chbitmask;
} __attribute__ ((packed));


static int hwsim_mon_xmit(struct sk_buff *skb, struct net_device *dev)
{
	/* TODO: allow packet injection */
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}


static void mac80211_hwsim_monitor_rx(struct ieee80211_hw *hw,
				      struct sk_buff *tx_skb)
{
	struct mac80211_hwsim_data *data = hw->priv;
	struct sk_buff *skb;
	struct hwsim_radiotap_hdr *hdr;
	u16 flags;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(tx_skb);
	struct ieee80211_rate *txrate = ieee80211_get_tx_rate(hw, info);

	if (!netif_running(hwsim_mon))
		return;

	skb = skb_copy_expand(tx_skb, sizeof(*hdr), 0, GFP_ATOMIC);
	if (skb == NULL)
		return;

	hdr = (struct hwsim_radiotap_hdr *) skb_push(skb, sizeof(*hdr));
	hdr->hdr.it_version = PKTHDR_RADIOTAP_VERSION;
	hdr->hdr.it_pad = 0;
	hdr->hdr.it_len = cpu_to_le16(sizeof(*hdr));
	hdr->hdr.it_present = cpu_to_le32((1 << IEEE80211_RADIOTAP_FLAGS) |
					  (1 << IEEE80211_RADIOTAP_RATE) |
					  (1 << IEEE80211_RADIOTAP_CHANNEL));
	hdr->rt_flags = 0;
	hdr->rt_rate = txrate->bitrate / 5;
	hdr->rt_channel = cpu_to_le16(data->channel->center_freq);
	flags = IEEE80211_CHAN_2GHZ;
	if (txrate->flags & IEEE80211_RATE_ERP_G)
		flags |= IEEE80211_CHAN_OFDM;
	else
		flags |= IEEE80211_CHAN_CCK;
	hdr->rt_chbitmask = cpu_to_le16(flags);

	skb->dev = hwsim_mon;
	skb_set_mac_header(skb, 0);
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_802_2);
	memset(skb->cb, 0, sizeof(skb->cb));
	netif_rx(skb);
}


static bool hwsim_ps_rx_ok(struct mac80211_hwsim_data *data,
			   struct sk_buff *skb)
{
	switch (data->ps) {
	case PS_DISABLED:
		return true;
	case PS_ENABLED:
		return false;
	case PS_AUTO_POLL:
		/* TODO: accept (some) Beacons by default and other frames only
		 * if pending PS-Poll has been sent */
		return true;
	case PS_MANUAL_POLL:
		/* Allow unicast frames to own address if there is a pending
		 * PS-Poll */
		if (data->ps_poll_pending &&
		    memcmp(data->hw->wiphy->perm_addr, skb->data + 4,
			   ETH_ALEN) == 0) {
			data->ps_poll_pending = false;
			return true;
		}
		return false;
	}

	return true;
}


static bool mac80211_hwsim_tx_frame(struct ieee80211_hw *hw,
				    struct sk_buff *skb)
{
	struct mac80211_hwsim_data *data = hw->priv, *data2;
	bool ack = false;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_rx_status rx_status;

	memset(&rx_status, 0, sizeof(rx_status));
	/* TODO: set mactime */
	rx_status.freq = data->channel->center_freq;
	rx_status.band = data->channel->band;
	rx_status.rate_idx = info->control.rates[0].idx;
	/* TODO: simulate signal strength (and optional packet drop) */

	if (data->ps != PS_DISABLED)
		hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_PM);

	/* Copy skb to all enabled radios that are on the current frequency */
	spin_lock(&hwsim_radio_lock);
	list_for_each_entry(data2, &hwsim_radios, list) {
		struct sk_buff *nskb;

		if (data == data2)
			continue;

		if (!data2->started || !hwsim_ps_rx_ok(data2, skb) ||
		    data->channel->center_freq != data2->channel->center_freq ||
		    !(data->group & data2->group))
			continue;

		nskb = skb_copy(skb, GFP_ATOMIC);
		if (nskb == NULL)
			continue;

		if (memcmp(hdr->addr1, data2->hw->wiphy->perm_addr,
			   ETH_ALEN) == 0)
			ack = true;
		ieee80211_rx_irqsafe(data2->hw, nskb, &rx_status);
	}
	spin_unlock(&hwsim_radio_lock);

	return ack;
}


static int mac80211_hwsim_tx(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	bool ack;
	struct ieee80211_tx_info *txi;

	mac80211_hwsim_monitor_rx(hw, skb);

	if (skb->len < 10) {
		/* Should not happen; just a sanity check for addr1 use */
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	ack = mac80211_hwsim_tx_frame(hw, skb);

	txi = IEEE80211_SKB_CB(skb);

	if (txi->control.vif)
		hwsim_check_magic(txi->control.vif);
	if (txi->control.sta)
		hwsim_check_sta_magic(txi->control.sta);

	ieee80211_tx_info_clear_status(txi);
	if (!(txi->flags & IEEE80211_TX_CTL_NO_ACK) && ack)
		txi->flags |= IEEE80211_TX_STAT_ACK;
	ieee80211_tx_status_irqsafe(hw, skb);
	return NETDEV_TX_OK;
}


static int mac80211_hwsim_start(struct ieee80211_hw *hw)
{
	struct mac80211_hwsim_data *data = hw->priv;
	printk(KERN_DEBUG "%s:%s\n", wiphy_name(hw->wiphy), __func__);
	data->started = 1;
	return 0;
}


static void mac80211_hwsim_stop(struct ieee80211_hw *hw)
{
	struct mac80211_hwsim_data *data = hw->priv;
	data->started = 0;
	del_timer(&data->beacon_timer);
	printk(KERN_DEBUG "%s:%s\n", wiphy_name(hw->wiphy), __func__);
}


static int mac80211_hwsim_add_interface(struct ieee80211_hw *hw,
					struct ieee80211_if_init_conf *conf)
{
	printk(KERN_DEBUG "%s:%s (type=%d mac_addr=%pM)\n",
	       wiphy_name(hw->wiphy), __func__, conf->type,
	       conf->mac_addr);
	hwsim_set_magic(conf->vif);
	return 0;
}


static void mac80211_hwsim_remove_interface(
	struct ieee80211_hw *hw, struct ieee80211_if_init_conf *conf)
{
	printk(KERN_DEBUG "%s:%s (type=%d mac_addr=%pM)\n",
	       wiphy_name(hw->wiphy), __func__, conf->type,
	       conf->mac_addr);
	hwsim_check_magic(conf->vif);
	hwsim_clear_magic(conf->vif);
}


static void mac80211_hwsim_beacon_tx(void *arg, u8 *mac,
				     struct ieee80211_vif *vif)
{
	struct ieee80211_hw *hw = arg;
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;

	hwsim_check_magic(vif);

	if (vif->type != NL80211_IFTYPE_AP &&
	    vif->type != NL80211_IFTYPE_MESH_POINT)
		return;

	skb = ieee80211_beacon_get(hw, vif);
	if (skb == NULL)
		return;
	info = IEEE80211_SKB_CB(skb);

	mac80211_hwsim_monitor_rx(hw, skb);
	mac80211_hwsim_tx_frame(hw, skb);
	dev_kfree_skb(skb);
}


static void mac80211_hwsim_beacon(unsigned long arg)
{
	struct ieee80211_hw *hw = (struct ieee80211_hw *) arg;
	struct mac80211_hwsim_data *data = hw->priv;

	if (!data->started)
		return;

	ieee80211_iterate_active_interfaces_atomic(
		hw, mac80211_hwsim_beacon_tx, hw);

	data->beacon_timer.expires = jiffies + data->beacon_int;
	add_timer(&data->beacon_timer);
}


static int mac80211_hwsim_config(struct ieee80211_hw *hw, u32 changed)
{
	struct mac80211_hwsim_data *data = hw->priv;
	struct ieee80211_conf *conf = &hw->conf;

	printk(KERN_DEBUG "%s:%s (freq=%d idle=%d ps=%d)\n",
	       wiphy_name(hw->wiphy), __func__,
	       conf->channel->center_freq,
	       !!(conf->flags & IEEE80211_CONF_IDLE),
	       !!(conf->flags & IEEE80211_CONF_PS));

	data->channel = conf->channel;
	if (!data->started || !data->beacon_int)
		del_timer(&data->beacon_timer);
	else
		mod_timer(&data->beacon_timer, jiffies + data->beacon_int);

	return 0;
}


static void mac80211_hwsim_configure_filter(struct ieee80211_hw *hw,
					    unsigned int changed_flags,
					    unsigned int *total_flags,
					    int mc_count,
					    struct dev_addr_list *mc_list)
{
	struct mac80211_hwsim_data *data = hw->priv;

	printk(KERN_DEBUG "%s:%s\n", wiphy_name(hw->wiphy), __func__);

	data->rx_filter = 0;
	if (*total_flags & FIF_PROMISC_IN_BSS)
		data->rx_filter |= FIF_PROMISC_IN_BSS;
	if (*total_flags & FIF_ALLMULTI)
		data->rx_filter |= FIF_ALLMULTI;

	*total_flags = data->rx_filter;
}

static void mac80211_hwsim_bss_info_changed(struct ieee80211_hw *hw,
					    struct ieee80211_vif *vif,
					    struct ieee80211_bss_conf *info,
					    u32 changed)
{
	struct hwsim_vif_priv *vp = (void *)vif->drv_priv;
	struct mac80211_hwsim_data *data = hw->priv;

	hwsim_check_magic(vif);

	printk(KERN_DEBUG "%s:%s(changed=0x%x)\n",
	       wiphy_name(hw->wiphy), __func__, changed);

	if (changed & BSS_CHANGED_BSSID) {
		printk(KERN_DEBUG "%s:%s: BSSID changed: %pM\n",
		       wiphy_name(hw->wiphy), __func__,
		       info->bssid);
		memcpy(vp->bssid, info->bssid, ETH_ALEN);
	}

	if (changed & BSS_CHANGED_ASSOC) {
		printk(KERN_DEBUG "  %s: ASSOC: assoc=%d aid=%d\n",
		       wiphy_name(hw->wiphy), info->assoc, info->aid);
		vp->assoc = info->assoc;
		vp->aid = info->aid;
	}

	if (changed & BSS_CHANGED_BEACON_INT) {
		printk(KERN_DEBUG "  %s: BCNINT: %d\n",
		       wiphy_name(hw->wiphy), info->beacon_int);
		data->beacon_int = 1024 * info->beacon_int / 1000 * HZ / 1000;
		if (WARN_ON(!data->beacon_int))
			data->beacon_int = 1;
	}

	if (changed & BSS_CHANGED_ERP_CTS_PROT) {
		printk(KERN_DEBUG "  %s: ERP_CTS_PROT: %d\n",
		       wiphy_name(hw->wiphy), info->use_cts_prot);
	}

	if (changed & BSS_CHANGED_ERP_PREAMBLE) {
		printk(KERN_DEBUG "  %s: ERP_PREAMBLE: %d\n",
		       wiphy_name(hw->wiphy), info->use_short_preamble);
	}

	if (changed & BSS_CHANGED_ERP_SLOT) {
		printk(KERN_DEBUG "  %s: ERP_SLOT: %d\n",
		       wiphy_name(hw->wiphy), info->use_short_slot);
	}

	if (changed & BSS_CHANGED_HT) {
		printk(KERN_DEBUG "  %s: HT: op_mode=0x%x\n",
		       wiphy_name(hw->wiphy),
		       info->ht_operation_mode);
	}

	if (changed & BSS_CHANGED_BASIC_RATES) {
		printk(KERN_DEBUG "  %s: BASIC_RATES: 0x%llx\n",
		       wiphy_name(hw->wiphy),
		       (unsigned long long) info->basic_rates);
	}
}

static void mac80211_hwsim_sta_notify(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif,
				      enum sta_notify_cmd cmd,
				      struct ieee80211_sta *sta)
{
	hwsim_check_magic(vif);
	switch (cmd) {
	case STA_NOTIFY_ADD:
		hwsim_set_sta_magic(sta);
		break;
	case STA_NOTIFY_REMOVE:
		hwsim_clear_sta_magic(sta);
		break;
	case STA_NOTIFY_SLEEP:
	case STA_NOTIFY_AWAKE:
		/* TODO: make good use of these flags */
		break;
	}
}

static int mac80211_hwsim_set_tim(struct ieee80211_hw *hw,
				  struct ieee80211_sta *sta,
				  bool set)
{
	hwsim_check_sta_magic(sta);
	return 0;
}

static int mac80211_hwsim_conf_tx(
	struct ieee80211_hw *hw, u16 queue,
	const struct ieee80211_tx_queue_params *params)
{
	printk(KERN_DEBUG "%s:%s (queue=%d txop=%d cw_min=%d cw_max=%d "
	       "aifs=%d)\n",
	       wiphy_name(hw->wiphy), __func__, queue,
	       params->txop, params->cw_min, params->cw_max, params->aifs);
	return 0;
}

static const struct ieee80211_ops mac80211_hwsim_ops =
{
	.tx = mac80211_hwsim_tx,
	.start = mac80211_hwsim_start,
	.stop = mac80211_hwsim_stop,
	.add_interface = mac80211_hwsim_add_interface,
	.remove_interface = mac80211_hwsim_remove_interface,
	.config = mac80211_hwsim_config,
	.configure_filter = mac80211_hwsim_configure_filter,
	.bss_info_changed = mac80211_hwsim_bss_info_changed,
	.sta_notify = mac80211_hwsim_sta_notify,
	.set_tim = mac80211_hwsim_set_tim,
	.conf_tx = mac80211_hwsim_conf_tx,
};


static void mac80211_hwsim_free(void)
{
	struct list_head tmplist, *i, *tmp;
	struct mac80211_hwsim_data *data;

	INIT_LIST_HEAD(&tmplist);

	spin_lock_bh(&hwsim_radio_lock);
	list_for_each_safe(i, tmp, &hwsim_radios)
		list_move(i, &tmplist);
	spin_unlock_bh(&hwsim_radio_lock);

	list_for_each_entry(data, &tmplist, list) {
		debugfs_remove(data->debugfs_group);
		debugfs_remove(data->debugfs_ps);
		debugfs_remove(data->debugfs);
		ieee80211_unregister_hw(data->hw);
		device_unregister(data->dev);
		ieee80211_free_hw(data->hw);
	}
	class_destroy(hwsim_class);
}


static struct device_driver mac80211_hwsim_driver = {
	.name = "mac80211_hwsim"
};

static const struct net_device_ops hwsim_netdev_ops = {
	.ndo_start_xmit 	= hwsim_mon_xmit,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static void hwsim_mon_setup(struct net_device *dev)
{
	dev->netdev_ops = &hwsim_netdev_ops;
	dev->destructor = free_netdev;
	ether_setup(dev);
	dev->tx_queue_len = 0;
	dev->type = ARPHRD_IEEE80211_RADIOTAP;
	memset(dev->dev_addr, 0, ETH_ALEN);
	dev->dev_addr[0] = 0x12;
}


static void hwsim_send_ps_poll(void *dat, u8 *mac, struct ieee80211_vif *vif)
{
	struct mac80211_hwsim_data *data = dat;
	struct hwsim_vif_priv *vp = (void *)vif->drv_priv;
	DECLARE_MAC_BUF(buf);
	struct sk_buff *skb;
	struct ieee80211_pspoll *pspoll;

	if (!vp->assoc)
		return;

	printk(KERN_DEBUG "%s:%s: send PS-Poll to %pM for aid %d\n",
	       wiphy_name(data->hw->wiphy), __func__, vp->bssid, vp->aid);

	skb = dev_alloc_skb(sizeof(*pspoll));
	if (!skb)
		return;
	pspoll = (void *) skb_put(skb, sizeof(*pspoll));
	pspoll->frame_control = cpu_to_le16(IEEE80211_FTYPE_CTL |
					    IEEE80211_STYPE_PSPOLL |
					    IEEE80211_FCTL_PM);
	pspoll->aid = cpu_to_le16(0xc000 | vp->aid);
	memcpy(pspoll->bssid, vp->bssid, ETH_ALEN);
	memcpy(pspoll->ta, mac, ETH_ALEN);
	if (!mac80211_hwsim_tx_frame(data->hw, skb))
		printk(KERN_DEBUG "%s: PS-Poll frame not ack'ed\n", __func__);
	dev_kfree_skb(skb);
}


static void hwsim_send_nullfunc(struct mac80211_hwsim_data *data, u8 *mac,
				struct ieee80211_vif *vif, int ps)
{
	struct hwsim_vif_priv *vp = (void *)vif->drv_priv;
	DECLARE_MAC_BUF(buf);
	struct sk_buff *skb;
	struct ieee80211_hdr *hdr;

	if (!vp->assoc)
		return;

	printk(KERN_DEBUG "%s:%s: send data::nullfunc to %pM ps=%d\n",
	       wiphy_name(data->hw->wiphy), __func__, vp->bssid, ps);

	skb = dev_alloc_skb(sizeof(*hdr));
	if (!skb)
		return;
	hdr = (void *) skb_put(skb, sizeof(*hdr) - ETH_ALEN);
	hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					 IEEE80211_STYPE_NULLFUNC |
					 (ps ? IEEE80211_FCTL_PM : 0));
	hdr->duration_id = cpu_to_le16(0);
	memcpy(hdr->addr1, vp->bssid, ETH_ALEN);
	memcpy(hdr->addr2, mac, ETH_ALEN);
	memcpy(hdr->addr3, vp->bssid, ETH_ALEN);
	if (!mac80211_hwsim_tx_frame(data->hw, skb))
		printk(KERN_DEBUG "%s: nullfunc frame not ack'ed\n", __func__);
	dev_kfree_skb(skb);
}


static void hwsim_send_nullfunc_ps(void *dat, u8 *mac,
				   struct ieee80211_vif *vif)
{
	struct mac80211_hwsim_data *data = dat;
	hwsim_send_nullfunc(data, mac, vif, 1);
}


static void hwsim_send_nullfunc_no_ps(void *dat, u8 *mac,
				      struct ieee80211_vif *vif)
{
	struct mac80211_hwsim_data *data = dat;
	hwsim_send_nullfunc(data, mac, vif, 0);
}


static int hwsim_fops_ps_read(void *dat, u64 *val)
{
	struct mac80211_hwsim_data *data = dat;
	*val = data->ps;
	return 0;
}

static int hwsim_fops_ps_write(void *dat, u64 val)
{
	struct mac80211_hwsim_data *data = dat;
	enum ps_mode old_ps;

	if (val != PS_DISABLED && val != PS_ENABLED && val != PS_AUTO_POLL &&
	    val != PS_MANUAL_POLL)
		return -EINVAL;

	old_ps = data->ps;
	data->ps = val;

	if (val == PS_MANUAL_POLL) {
		ieee80211_iterate_active_interfaces(data->hw,
						    hwsim_send_ps_poll, data);
		data->ps_poll_pending = true;
	} else if (old_ps == PS_DISABLED && val != PS_DISABLED) {
		ieee80211_iterate_active_interfaces(data->hw,
						    hwsim_send_nullfunc_ps,
						    data);
	} else if (old_ps != PS_DISABLED && val == PS_DISABLED) {
		ieee80211_iterate_active_interfaces(data->hw,
						    hwsim_send_nullfunc_no_ps,
						    data);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hwsim_fops_ps, hwsim_fops_ps_read, hwsim_fops_ps_write,
			"%llu\n");


static int hwsim_fops_group_read(void *dat, u64 *val)
{
	struct mac80211_hwsim_data *data = dat;
	*val = data->group;
	return 0;
}

static int hwsim_fops_group_write(void *dat, u64 val)
{
	struct mac80211_hwsim_data *data = dat;
	data->group = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hwsim_fops_group,
			hwsim_fops_group_read, hwsim_fops_group_write,
			"%llx\n");

static int __init init_mac80211_hwsim(void)
{
	int i, err = 0;
	u8 addr[ETH_ALEN];
	struct mac80211_hwsim_data *data;
	struct ieee80211_hw *hw;
	enum ieee80211_band band;

	if (radios < 1 || radios > 100)
		return -EINVAL;

	spin_lock_init(&hwsim_radio_lock);
	INIT_LIST_HEAD(&hwsim_radios);

	hwsim_class = class_create(THIS_MODULE, "mac80211_hwsim");
	if (IS_ERR(hwsim_class))
		return PTR_ERR(hwsim_class);

	memset(addr, 0, ETH_ALEN);
	addr[0] = 0x02;

	for (i = 0; i < radios; i++) {
		printk(KERN_DEBUG "mac80211_hwsim: Initializing radio %d\n",
		       i);
		hw = ieee80211_alloc_hw(sizeof(*data), &mac80211_hwsim_ops);
		if (!hw) {
			printk(KERN_DEBUG "mac80211_hwsim: ieee80211_alloc_hw "
			       "failed\n");
			err = -ENOMEM;
			goto failed;
		}
		data = hw->priv;
		data->hw = hw;

		data->dev = device_create(hwsim_class, NULL, 0, hw,
					  "hwsim%d", i);
		if (IS_ERR(data->dev)) {
			printk(KERN_DEBUG
			       "mac80211_hwsim: device_create "
			       "failed (%ld)\n", PTR_ERR(data->dev));
			err = -ENOMEM;
			goto failed_drvdata;
		}
		data->dev->driver = &mac80211_hwsim_driver;

		SET_IEEE80211_DEV(hw, data->dev);
		addr[3] = i >> 8;
		addr[4] = i;
		SET_IEEE80211_PERM_ADDR(hw, addr);

		hw->channel_change_time = 1;
		hw->queues = 4;
		hw->wiphy->interface_modes =
			BIT(NL80211_IFTYPE_STATION) |
			BIT(NL80211_IFTYPE_AP) |
			BIT(NL80211_IFTYPE_MESH_POINT);

		hw->flags = IEEE80211_HW_MFP_CAPABLE;

		/* ask mac80211 to reserve space for magic */
		hw->vif_data_size = sizeof(struct hwsim_vif_priv);
		hw->sta_data_size = sizeof(struct hwsim_sta_priv);

		memcpy(data->channels_2ghz, hwsim_channels_2ghz,
			sizeof(hwsim_channels_2ghz));
		memcpy(data->channels_5ghz, hwsim_channels_5ghz,
			sizeof(hwsim_channels_5ghz));
		memcpy(data->rates, hwsim_rates, sizeof(hwsim_rates));

		for (band = IEEE80211_BAND_2GHZ; band < IEEE80211_NUM_BANDS; band++) {
			struct ieee80211_supported_band *sband = &data->bands[band];
			switch (band) {
			case IEEE80211_BAND_2GHZ:
				sband->channels = data->channels_2ghz;
				sband->n_channels =
					ARRAY_SIZE(hwsim_channels_2ghz);
				break;
			case IEEE80211_BAND_5GHZ:
				sband->channels = data->channels_5ghz;
				sband->n_channels =
					ARRAY_SIZE(hwsim_channels_5ghz);
				break;
			default:
				break;
			}

			sband->bitrates = data->rates;
			sband->n_bitrates = ARRAY_SIZE(hwsim_rates);

			sband->ht_cap.ht_supported = true;
			sband->ht_cap.cap = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				IEEE80211_HT_CAP_GRN_FLD |
				IEEE80211_HT_CAP_SGI_40 |
				IEEE80211_HT_CAP_DSSSCCK40;
			sband->ht_cap.ampdu_factor = 0x3;
			sband->ht_cap.ampdu_density = 0x6;
			memset(&sband->ht_cap.mcs, 0,
			       sizeof(sband->ht_cap.mcs));
			sband->ht_cap.mcs.rx_mask[0] = 0xff;
			sband->ht_cap.mcs.rx_mask[1] = 0xff;
			sband->ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;

			hw->wiphy->bands[band] = sband;
		}
		/* By default all radios are belonging to the first group */
		data->group = 1;

		/* Work to be done prior to ieee80211_register_hw() */
		switch (regtest) {
		case HWSIM_REGTEST_DISABLED:
		case HWSIM_REGTEST_DRIVER_REG_FOLLOW:
		case HWSIM_REGTEST_DRIVER_REG_ALL:
		case HWSIM_REGTEST_DIFF_COUNTRY:
			/*
			 * Nothing to be done for driver regulatory domain
			 * hints prior to ieee80211_register_hw()
			 */
			break;
		case HWSIM_REGTEST_WORLD_ROAM:
			if (i == 0) {
				hw->wiphy->custom_regulatory = true;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&hwsim_world_regdom_custom_01);
			}
			break;
		case HWSIM_REGTEST_CUSTOM_WORLD:
			hw->wiphy->custom_regulatory = true;
			wiphy_apply_custom_regulatory(hw->wiphy,
				&hwsim_world_regdom_custom_01);
			break;
		case HWSIM_REGTEST_CUSTOM_WORLD_2:
			if (i == 0) {
				hw->wiphy->custom_regulatory = true;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&hwsim_world_regdom_custom_01);
			} else if (i == 1) {
				hw->wiphy->custom_regulatory = true;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&hwsim_world_regdom_custom_02);
			}
			break;
		case HWSIM_REGTEST_STRICT_ALL:
			hw->wiphy->strict_regulatory = true;
			break;
		case HWSIM_REGTEST_STRICT_FOLLOW:
		case HWSIM_REGTEST_STRICT_AND_DRIVER_REG:
			if (i == 0)
				hw->wiphy->strict_regulatory = true;
			break;
		case HWSIM_REGTEST_ALL:
			if (i == 0) {
				hw->wiphy->custom_regulatory = true;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&hwsim_world_regdom_custom_01);
			} else if (i == 1) {
				hw->wiphy->custom_regulatory = true;
				wiphy_apply_custom_regulatory(hw->wiphy,
					&hwsim_world_regdom_custom_02);
			} else if (i == 4)
				hw->wiphy->strict_regulatory = true;
			break;
		default:
			break;
		}

		/* give the regulatory workqueue a chance to run */
		if (regtest)
			schedule_timeout_interruptible(1);
		err = ieee80211_register_hw(hw);
		if (err < 0) {
			printk(KERN_DEBUG "mac80211_hwsim: "
			       "ieee80211_register_hw failed (%d)\n", err);
			goto failed_hw;
		}

		/* Work to be done after to ieee80211_register_hw() */
		switch (regtest) {
		case HWSIM_REGTEST_WORLD_ROAM:
		case HWSIM_REGTEST_DISABLED:
			break;
		case HWSIM_REGTEST_DRIVER_REG_FOLLOW:
			if (!i)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[0]);
			break;
		case HWSIM_REGTEST_DRIVER_REG_ALL:
		case HWSIM_REGTEST_STRICT_ALL:
			regulatory_hint(hw->wiphy, hwsim_alpha2s[0]);
			break;
		case HWSIM_REGTEST_DIFF_COUNTRY:
			if (i < ARRAY_SIZE(hwsim_alpha2s))
				regulatory_hint(hw->wiphy, hwsim_alpha2s[i]);
			break;
		case HWSIM_REGTEST_CUSTOM_WORLD:
		case HWSIM_REGTEST_CUSTOM_WORLD_2:
			/*
			 * Nothing to be done for custom world regulatory
			 * domains after to ieee80211_register_hw
			 */
			break;
		case HWSIM_REGTEST_STRICT_FOLLOW:
			if (i == 0)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[0]);
			break;
		case HWSIM_REGTEST_STRICT_AND_DRIVER_REG:
			if (i == 0)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[0]);
			else if (i == 1)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[1]);
			break;
		case HWSIM_REGTEST_ALL:
			if (i == 2)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[0]);
			else if (i == 3)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[1]);
			else if (i == 4)
				regulatory_hint(hw->wiphy, hwsim_alpha2s[2]);
			break;
		default:
			break;
		}

		printk(KERN_DEBUG "%s: hwaddr %pM registered\n",
		       wiphy_name(hw->wiphy),
		       hw->wiphy->perm_addr);

		data->debugfs = debugfs_create_dir("hwsim",
						   hw->wiphy->debugfsdir);
		data->debugfs_ps = debugfs_create_file("ps", 0666,
						       data->debugfs, data,
						       &hwsim_fops_ps);
		data->debugfs_group = debugfs_create_file("group", 0666,
							data->debugfs, data,
							&hwsim_fops_group);

		setup_timer(&data->beacon_timer, mac80211_hwsim_beacon,
			    (unsigned long) hw);

		list_add_tail(&data->list, &hwsim_radios);
	}

	hwsim_mon = alloc_netdev(0, "hwsim%d", hwsim_mon_setup);
	if (hwsim_mon == NULL)
		goto failed;

	rtnl_lock();

	err = dev_alloc_name(hwsim_mon, hwsim_mon->name);
	if (err < 0)
		goto failed_mon;


	err = register_netdevice(hwsim_mon);
	if (err < 0)
		goto failed_mon;

	rtnl_unlock();

	return 0;

failed_mon:
	rtnl_unlock();
	free_netdev(hwsim_mon);
	mac80211_hwsim_free();
	return err;

failed_hw:
	device_unregister(data->dev);
failed_drvdata:
	ieee80211_free_hw(hw);
failed:
	mac80211_hwsim_free();
	return err;
}


static void __exit exit_mac80211_hwsim(void)
{
	printk(KERN_DEBUG "mac80211_hwsim: unregister radios\n");

	unregister_netdev(hwsim_mon);
	mac80211_hwsim_free();
}


module_init(init_mac80211_hwsim);
module_exit(exit_mac80211_hwsim);
