/*
 * 802_3
 *
 * Author:
 * Chris Vitale csv@bluetail.com
 *
 * May 2003
 *
 */
#include <linux/module.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_802_3.h>

static bool
ebt_802_3_mt(const struct sk_buff *skb, const struct xt_match_param *par)
{
	const struct ebt_802_3_info *info = par->matchinfo;
	const struct ebt_802_3_hdr *hdr = ebt_802_3_hdr(skb);
	__be16 type = hdr->llc.ui.ctrl & IS_UI ? hdr->llc.ui.type : hdr->llc.ni.type;

	if (info->bitmask & EBT_802_3_SAP) {
		if (FWINV(info->sap != hdr->llc.ui.ssap, EBT_802_3_SAP))
			return false;
		if (FWINV(info->sap != hdr->llc.ui.dsap, EBT_802_3_SAP))
			return false;
	}

	if (info->bitmask & EBT_802_3_TYPE) {
		if (!(hdr->llc.ui.dsap == CHECK_TYPE && hdr->llc.ui.ssap == CHECK_TYPE))
			return false;
		if (FWINV(info->type != type, EBT_802_3_TYPE))
			return false;
	}

	return true;
}

static bool ebt_802_3_mt_check(const struct xt_mtchk_param *par)
{
	const struct ebt_802_3_info *info = par->matchinfo;

	if (info->bitmask & ~EBT_802_3_MASK || info->invflags & ~EBT_802_3_MASK)
		return false;

	return true;
}

static struct xt_match ebt_802_3_mt_reg __read_mostly = {
	.name		= "802_3",
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
	.match		= ebt_802_3_mt,
	.checkentry	= ebt_802_3_mt_check,
	.matchsize	= XT_ALIGN(sizeof(struct ebt_802_3_info)),
	.me		= THIS_MODULE,
};

static int __init ebt_802_3_init(void)
{
	return xt_register_match(&ebt_802_3_mt_reg);
}

static void __exit ebt_802_3_fini(void)
{
	xt_unregister_match(&ebt_802_3_mt_reg);
}

module_init(ebt_802_3_init);
module_exit(ebt_802_3_fini);
MODULE_DESCRIPTION("Ebtables: DSAP/SSAP field and SNAP type matching");
MODULE_LICENSE("GPL");
