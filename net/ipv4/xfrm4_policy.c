/*
 * xfrm4_policy.c
 *
 * Changes:
 *	Kazunori MIYAZAWA @USAGI
 * 	YOSHIFUJI Hideaki @USAGI
 *		Split up af-specific portion
 *
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/inetdevice.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <net/ip.h>

static struct dst_ops xfrm4_dst_ops;
static struct xfrm_policy_afinfo xfrm4_policy_afinfo;

static struct dst_entry *xfrm4_dst_lookup(int tos, xfrm_address_t *saddr,
					  xfrm_address_t *daddr)
{
	struct flowi fl = {
		.nl_u = {
			.ip4_u = {
				.tos = tos,
				.daddr = daddr->a4,
			},
		},
	};
	struct dst_entry *dst;
	struct rtable *rt;
	int err;

	if (saddr)
		fl.fl4_src = saddr->a4;

	err = __ip_route_output_key(&rt, &fl);
	dst = &rt->u.dst;
	if (err)
		dst = ERR_PTR(err);
	return dst;
}

static int xfrm4_get_saddr(xfrm_address_t *saddr, xfrm_address_t *daddr)
{
	struct dst_entry *dst;
	struct rtable *rt;

	dst = xfrm4_dst_lookup(0, NULL, daddr);
	if (IS_ERR(dst))
		return -EHOSTUNREACH;

	rt = (struct rtable *)dst;
	saddr->a4 = rt->rt_src;
	dst_release(dst);
	return 0;
}

static struct dst_entry *
__xfrm4_find_bundle(struct flowi *fl, struct xfrm_policy *policy)
{
	struct dst_entry *dst;

	read_lock_bh(&policy->lock);
	for (dst = policy->bundles; dst; dst = dst->next) {
		struct xfrm_dst *xdst = (struct xfrm_dst*)dst;
		if (xdst->u.rt.fl.oif == fl->oif &&	/*XXX*/
		    xdst->u.rt.fl.fl4_dst == fl->fl4_dst &&
		    xdst->u.rt.fl.fl4_src == fl->fl4_src &&
		    xdst->u.rt.fl.fl4_tos == fl->fl4_tos &&
		    xfrm_bundle_ok(policy, xdst, fl, AF_INET, 0)) {
			dst_clone(dst);
			break;
		}
	}
	read_unlock_bh(&policy->lock);
	return dst;
}

/* Allocate chain of dst_entry's, attach known xfrm's, calculate
 * all the metrics... Shortly, bundle a bundle.
 */

static int
__xfrm4_bundle_create(struct xfrm_policy *policy, struct xfrm_state **xfrm, int nx,
		      struct flowi *fl, struct dst_entry **dst_p)
{
	struct dst_entry *dst, *dst_prev;
	struct rtable *rt0 = (struct rtable*)(*dst_p);
	struct rtable *rt = rt0;
	int tos = fl->fl4_tos;
	int i;
	int err;
	int header_len = 0;
	int trailer_len = 0;

	dst = dst_prev = NULL;
	dst_hold(&rt->u.dst);

	for (i = 0; i < nx; i++) {
		struct dst_entry *dst1 = dst_alloc(&xfrm4_dst_ops);
		struct xfrm_dst *xdst;

		if (unlikely(dst1 == NULL)) {
			err = -ENOBUFS;
			dst_release(&rt->u.dst);
			goto error;
		}

		if (!dst)
			dst = dst1;
		else {
			dst_prev->child = dst1;
			dst1->flags |= DST_NOHASH;
			dst_clone(dst1);
		}

		xdst = (struct xfrm_dst *)dst1;
		xdst->route = &rt->u.dst;
		xdst->genid = xfrm[i]->genid;

		dst1->next = dst_prev;
		dst_prev = dst1;

		header_len += xfrm[i]->props.header_len;
		trailer_len += xfrm[i]->props.trailer_len;

		if (xfrm[i]->props.mode != XFRM_MODE_TRANSPORT) {
			dst1 = xfrm_dst_lookup(xfrm[i], tos);
			err = PTR_ERR(dst1);
			if (IS_ERR(dst1))
				goto error;

			rt = (struct rtable *)dst1;
		} else
			dst_hold(&rt->u.dst);
	}

	dst_prev->child = &rt->u.dst;
	dst->path = &rt->u.dst;

	/* Copy neighbout for reachability confirmation */
	dst->neighbour = neigh_clone(rt->u.dst.neighbour);

	*dst_p = dst;
	dst = dst_prev;

	dst_prev = *dst_p;
	i = 0;
	err = -ENODEV;
	for (; dst_prev != &rt->u.dst; dst_prev = dst_prev->child) {
		struct xfrm_dst *x = (struct xfrm_dst*)dst_prev;
		x->u.rt.fl = *fl;

		dst_prev->xfrm = xfrm[i++];
		dst_prev->dev = rt->u.dst.dev;
		if (!rt->u.dst.dev)
			goto error;
		dev_hold(rt->u.dst.dev);

		x->u.rt.idev = in_dev_get(rt->u.dst.dev);
		if (!x->u.rt.idev)
			goto error;

		dst_prev->obsolete	= -1;
		dst_prev->flags	       |= DST_HOST;
		dst_prev->lastuse	= jiffies;
		dst_prev->header_len	= header_len;
		dst_prev->trailer_len	= trailer_len;
		memcpy(&dst_prev->metrics, &x->route->metrics, sizeof(dst_prev->metrics));

		dst_prev->input = dst_discard;
		dst_prev->output = dst_prev->xfrm->outer_mode->afinfo->output;
		if (rt0->peer)
			atomic_inc(&rt0->peer->refcnt);
		x->u.rt.peer = rt0->peer;
		/* Sheit... I remember I did this right. Apparently,
		 * it was magically lost, so this code needs audit */
		x->u.rt.rt_flags = rt0->rt_flags&(RTCF_BROADCAST|RTCF_MULTICAST|RTCF_LOCAL);
		x->u.rt.rt_type = rt0->rt_type;
		x->u.rt.rt_src = rt0->rt_src;
		x->u.rt.rt_dst = rt0->rt_dst;
		x->u.rt.rt_gateway = rt0->rt_gateway;
		x->u.rt.rt_spec_dst = rt0->rt_spec_dst;
		header_len -= x->u.dst.xfrm->props.header_len;
		trailer_len -= x->u.dst.xfrm->props.trailer_len;
	}

	xfrm_init_pmtu(dst);
	return 0;

error:
	if (dst)
		dst_free(dst);
	return err;
}

static void
_decode_session4(struct sk_buff *skb, struct flowi *fl)
{
	struct iphdr *iph = ip_hdr(skb);
	u8 *xprth = skb_network_header(skb) + iph->ihl * 4;

	memset(fl, 0, sizeof(struct flowi));
	if (!(iph->frag_off & htons(IP_MF | IP_OFFSET))) {
		switch (iph->protocol) {
		case IPPROTO_UDP:
		case IPPROTO_UDPLITE:
		case IPPROTO_TCP:
		case IPPROTO_SCTP:
		case IPPROTO_DCCP:
			if (pskb_may_pull(skb, xprth + 4 - skb->data)) {
				__be16 *ports = (__be16 *)xprth;

				fl->fl_ip_sport = ports[0];
				fl->fl_ip_dport = ports[1];
			}
			break;

		case IPPROTO_ICMP:
			if (pskb_may_pull(skb, xprth + 2 - skb->data)) {
				u8 *icmp = xprth;

				fl->fl_icmp_type = icmp[0];
				fl->fl_icmp_code = icmp[1];
			}
			break;

		case IPPROTO_ESP:
			if (pskb_may_pull(skb, xprth + 4 - skb->data)) {
				__be32 *ehdr = (__be32 *)xprth;

				fl->fl_ipsec_spi = ehdr[0];
			}
			break;

		case IPPROTO_AH:
			if (pskb_may_pull(skb, xprth + 8 - skb->data)) {
				__be32 *ah_hdr = (__be32*)xprth;

				fl->fl_ipsec_spi = ah_hdr[1];
			}
			break;

		case IPPROTO_COMP:
			if (pskb_may_pull(skb, xprth + 4 - skb->data)) {
				__be16 *ipcomp_hdr = (__be16 *)xprth;

				fl->fl_ipsec_spi = htonl(ntohs(ipcomp_hdr[1]));
			}
			break;
		default:
			fl->fl_ipsec_spi = 0;
			break;
		}
	}
	fl->proto = iph->protocol;
	fl->fl4_dst = iph->daddr;
	fl->fl4_src = iph->saddr;
	fl->fl4_tos = iph->tos;
}

static inline int xfrm4_garbage_collect(void)
{
	xfrm4_policy_afinfo.garbage_collect();
	return (atomic_read(&xfrm4_dst_ops.entries) > xfrm4_dst_ops.gc_thresh*2);
}

static void xfrm4_update_pmtu(struct dst_entry *dst, u32 mtu)
{
	struct xfrm_dst *xdst = (struct xfrm_dst *)dst;
	struct dst_entry *path = xdst->route;

	path->ops->update_pmtu(path, mtu);
}

static void xfrm4_dst_destroy(struct dst_entry *dst)
{
	struct xfrm_dst *xdst = (struct xfrm_dst *)dst;

	if (likely(xdst->u.rt.idev))
		in_dev_put(xdst->u.rt.idev);
	if (likely(xdst->u.rt.peer))
		inet_putpeer(xdst->u.rt.peer);
	xfrm_dst_destroy(xdst);
}

static void xfrm4_dst_ifdown(struct dst_entry *dst, struct net_device *dev,
			     int unregister)
{
	struct xfrm_dst *xdst;

	if (!unregister)
		return;

	xdst = (struct xfrm_dst *)dst;
	if (xdst->u.rt.idev->dev == dev) {
		struct in_device *loopback_idev = in_dev_get(init_net.loopback_dev);
		BUG_ON(!loopback_idev);

		do {
			in_dev_put(xdst->u.rt.idev);
			xdst->u.rt.idev = loopback_idev;
			in_dev_hold(loopback_idev);
			xdst = (struct xfrm_dst *)xdst->u.dst.child;
		} while (xdst->u.dst.xfrm);

		__in_dev_put(loopback_idev);
	}

	xfrm_dst_ifdown(dst, dev);
}

static struct dst_ops xfrm4_dst_ops = {
	.family =		AF_INET,
	.protocol =		__constant_htons(ETH_P_IP),
	.gc =			xfrm4_garbage_collect,
	.update_pmtu =		xfrm4_update_pmtu,
	.destroy =		xfrm4_dst_destroy,
	.ifdown =		xfrm4_dst_ifdown,
	.gc_thresh =		1024,
	.entry_size =		sizeof(struct xfrm_dst),
};

static struct xfrm_policy_afinfo xfrm4_policy_afinfo = {
	.family = 		AF_INET,
	.dst_ops =		&xfrm4_dst_ops,
	.dst_lookup =		xfrm4_dst_lookup,
	.get_saddr =		xfrm4_get_saddr,
	.find_bundle = 		__xfrm4_find_bundle,
	.bundle_create =	__xfrm4_bundle_create,
	.decode_session =	_decode_session4,
};

static void __init xfrm4_policy_init(void)
{
	xfrm_policy_register_afinfo(&xfrm4_policy_afinfo);
}

static void __exit xfrm4_policy_fini(void)
{
	xfrm_policy_unregister_afinfo(&xfrm4_policy_afinfo);
}

void __init xfrm4_init(void)
{
	xfrm4_state_init();
	xfrm4_policy_init();
}

