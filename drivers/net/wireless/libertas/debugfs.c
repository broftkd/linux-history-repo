#include <linux/module.h>
#include <linux/dcache.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <net/iw_handler.h>

#include "dev.h"
#include "decl.h"
#include "host.h"
#include "debugfs.h"

static struct dentry *lbs_dir;
static char *szStates[] = {
	"Connected",
	"Disconnected"
};

#ifdef PROC_DEBUG
static void lbs_debug_init(struct lbs_private *priv, struct net_device *dev);
#endif

static int open_file_generic(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t write_file_dummy(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
        return -EINVAL;
}

static const size_t len = PAGE_SIZE;

static ssize_t lbs_dev_info(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	size_t pos = 0;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;
	ssize_t res;

	pos += snprintf(buf+pos, len-pos, "state = %s\n",
				szStates[priv->adapter->connect_status]);
	pos += snprintf(buf+pos, len-pos, "region_code = %02x\n",
				(u32) priv->adapter->regioncode);

	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);

	free_page(addr);
	return res;
}


static ssize_t lbs_getscantable(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	size_t pos = 0;
	int numscansdone = 0, res;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;
	DECLARE_MAC_BUF(mac);
	struct bss_descriptor * iter_bss;

	pos += snprintf(buf+pos, len-pos,
		"# | ch  | rssi |       bssid       |   cap    | Qual | SSID \n");

	mutex_lock(&priv->adapter->lock);
	list_for_each_entry (iter_bss, &priv->adapter->network_list, list) {
		u16 ibss = (iter_bss->capability & WLAN_CAPABILITY_IBSS);
		u16 privacy = (iter_bss->capability & WLAN_CAPABILITY_PRIVACY);
		u16 spectrum_mgmt = (iter_bss->capability & WLAN_CAPABILITY_SPECTRUM_MGMT);

		pos += snprintf(buf+pos, len-pos,
			"%02u| %03d | %04ld | %s |",
			numscansdone, iter_bss->channel, iter_bss->rssi,
			print_mac(mac, iter_bss->bssid));
		pos += snprintf(buf+pos, len-pos, " %04x-", iter_bss->capability);
		pos += snprintf(buf+pos, len-pos, "%c%c%c |",
				ibss ? 'A' : 'I', privacy ? 'P' : ' ',
				spectrum_mgmt ? 'S' : ' ');
		pos += snprintf(buf+pos, len-pos, " %04d |", SCAN_RSSI(iter_bss->rssi));
		pos += snprintf(buf+pos, len-pos, " %s\n",
		                escape_essid(iter_bss->ssid, iter_bss->ssid_len));

		numscansdone++;
	}
	mutex_unlock(&priv->adapter->lock);

	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);

	free_page(addr);
	return res;
}

static ssize_t lbs_sleepparams_write(struct file *file,
				const char __user *user_buf, size_t count,
				loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t buf_size, res;
	int p1, p2, p3, p4, p5, p6;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, user_buf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	res = sscanf(buf, "%d %d %d %d %d %d", &p1, &p2, &p3, &p4, &p5, &p6);
	if (res != 6) {
		res = -EFAULT;
		goto out_unlock;
	}
	priv->adapter->sp.sp_error = p1;
	priv->adapter->sp.sp_offset = p2;
	priv->adapter->sp.sp_stabletime = p3;
	priv->adapter->sp.sp_calcontrol = p4;
	priv->adapter->sp.sp_extsleepclk = p5;
	priv->adapter->sp.sp_reserved = p6;

	res = lbs_prepare_and_send_command(priv,
				CMD_802_11_SLEEP_PARAMS,
				CMD_ACT_SET,
				CMD_OPTION_WAITFORRSP, 0, NULL);

	if (!res)
		res = count;
	else
		res = -EINVAL;

out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_sleepparams_read(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	struct lbs_adapter *adapter = priv->adapter;
	ssize_t res;
	size_t pos = 0;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	res = lbs_prepare_and_send_command(priv,
				CMD_802_11_SLEEP_PARAMS,
				CMD_ACT_GET,
				CMD_OPTION_WAITFORRSP, 0, NULL);
	if (res) {
		res = -EFAULT;
		goto out_unlock;
	}

	pos += snprintf(buf, len, "%d %d %d %d %d %d\n", adapter->sp.sp_error,
			adapter->sp.sp_offset, adapter->sp.sp_stabletime,
			adapter->sp.sp_calcontrol, adapter->sp.sp_extsleepclk,
			adapter->sp.sp_reserved);

	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);

out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_extscan(struct file *file, const char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	union iwreq_data wrqu;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}

	lbs_send_specific_ssid_scan(priv, buf, strlen(buf)-1, 0);

	memset(&wrqu, 0, sizeof(union iwreq_data));
	wireless_send_event(priv->dev, SIOCGIWSCAN, &wrqu, NULL);

out_unlock:
	free_page(addr);
	return count;
}

static void lbs_parse_bssid(char *buf, size_t count,
	struct lbs_ioctl_user_scan_cfg *scan_cfg)
{
	char *hold;
	unsigned int mac[ETH_ALEN];

	hold = strstr(buf, "bssid=");
	if (!hold)
		return;
	hold += 6;
	sscanf(hold, "%02x:%02x:%02x:%02x:%02x:%02x",
	       mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	memcpy(scan_cfg->bssid, mac, ETH_ALEN);
}

static void lbs_parse_ssid(char *buf, size_t count,
	struct lbs_ioctl_user_scan_cfg *scan_cfg)
{
	char *hold, *end;
	ssize_t size;

	hold = strstr(buf, "ssid=");
	if (!hold)
		return;
	hold += 5;
	end = strchr(hold, ' ');
	if (!end)
		end = buf + count - 1;

	size = min((size_t)IW_ESSID_MAX_SIZE, (size_t) (end - hold));
	strncpy(scan_cfg->ssid, hold, size);

	return;
}

static int lbs_parse_clear(char *buf, size_t count, const char *tag)
{
	char *hold;
	int val;

	hold = strstr(buf, tag);
	if (!hold)
		return 0;
	hold += strlen(tag);
	sscanf(hold, "%d", &val);

	if (val != 0)
		val = 1;

	return val;
}

static int lbs_parse_dur(char *buf, size_t count,
	struct lbs_ioctl_user_scan_cfg *scan_cfg)
{
	char *hold;
	int val;

	hold = strstr(buf, "dur=");
	if (!hold)
		return 0;
	hold += 4;
	sscanf(hold, "%d", &val);

	return val;
}

static void lbs_parse_type(char *buf, size_t count,
	struct lbs_ioctl_user_scan_cfg *scan_cfg)
{
	char *hold;
	int val;

	hold = strstr(buf, "type=");
	if (!hold)
		return;
	hold += 5;
	sscanf(hold, "%d", &val);

	/* type=1,2 or 3 */
	if (val < 1 || val > 3)
		return;

	scan_cfg->bsstype = val;

	return;
}

static ssize_t lbs_setuserscan(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	struct lbs_ioctl_user_scan_cfg *scan_cfg;
	union iwreq_data wrqu;
	int dur;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	scan_cfg = kzalloc(sizeof(struct lbs_ioctl_user_scan_cfg), GFP_KERNEL);
	if (!scan_cfg)
		return -ENOMEM;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}

	scan_cfg->bsstype = LBS_SCAN_BSS_TYPE_ANY;

	dur = lbs_parse_dur(buf, count, scan_cfg);
	lbs_parse_bssid(buf, count, scan_cfg);
	scan_cfg->clear_bssid = lbs_parse_clear(buf, count, "clear_bssid=");
	lbs_parse_ssid(buf, count, scan_cfg);
	scan_cfg->clear_ssid = lbs_parse_clear(buf, count, "clear_ssid=");
	lbs_parse_type(buf, count, scan_cfg);

	lbs_scan_networks(priv, scan_cfg, 1);
	wait_event_interruptible(priv->adapter->cmd_pending,
				 !priv->adapter->nr_cmd_pending);

	memset(&wrqu, 0x00, sizeof(union iwreq_data));
	wireless_send_event(priv->dev, SIOCGIWSCAN, &wrqu, NULL);

out_unlock:
	free_page(addr);
	kfree(scan_cfg);
	return count;
}


/*
 * When calling CMD_802_11_SUBSCRIBE_EVENT with CMD_ACT_GET, me might
 * get a bunch of vendor-specific TLVs (a.k.a. IEs) back from the
 * firmware. Here's an example:
 *	04 01 02 00 00 00 05 01 02 00 00 00 06 01 02 00
 *	00 00 07 01 02 00 3c 00 00 00 00 00 00 00 03 03
 *	00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 *
 * The 04 01 is the TLV type (here TLV_TYPE_RSSI_LOW), 02 00 is the length,
 * 00 00 are the data bytes of this TLV. For this TLV, their meaning is
 * defined in mrvlietypes_thresholds
 *
 * This function searches in this TLV data chunk for a given TLV type
 * and returns a pointer to the first data byte of the TLV, or to NULL
 * if the TLV hasn't been found.
 */
static void *lbs_tlv_find(u16 tlv_type, const u8 *tlv, u16 size)
{
	__le16 le_type = cpu_to_le16(tlv_type);
	ssize_t pos = 0;
	struct mrvlietypesheader *tlv_h;
	while (pos < size) {
		u16 length;
		tlv_h = (struct mrvlietypesheader *) tlv;
		if (tlv_h->type == le_type)
			return tlv_h;
		if (tlv_h->len == 0)
			return NULL;
		length = le16_to_cpu(tlv_h->len) +
			sizeof(struct mrvlietypesheader);
		pos += length;
		tlv += length;
	}
	return NULL;
}


/*
 * This just gets the bitmap of currently subscribed events. Used when
 * adding an additonal event subscription.
 */
static u16 lbs_get_events_bitmap(struct lbs_private *priv)
{
	ssize_t res;

	struct cmd_ds_802_11_subscribe_event *events = kzalloc(
		sizeof(struct cmd_ds_802_11_subscribe_event),
		GFP_KERNEL);

	res = lbs_prepare_and_send_command(priv,
			CMD_802_11_SUBSCRIBE_EVENT, CMD_ACT_GET,
			CMD_OPTION_WAITFORRSP, 0, events);

	if (res) {
		kfree(events);
		return 0;
	}
	return le16_to_cpu(events->events);
}


static ssize_t lbs_threshold_read(
	u16 tlv_type, u16 event_mask,
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res = 0;
	size_t pos = 0;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;
	u8 value;
	u8 freq;

	struct cmd_ds_802_11_subscribe_event *events = kzalloc(
		sizeof(struct cmd_ds_802_11_subscribe_event),
		GFP_KERNEL);
	struct mrvlietypes_thresholds *got;

	res = lbs_prepare_and_send_command(priv,
			CMD_802_11_SUBSCRIBE_EVENT, CMD_ACT_GET,
			CMD_OPTION_WAITFORRSP, 0, events);
	if (res) {
		kfree(events);
		return res;
	}

	got = lbs_tlv_find(tlv_type, events->tlv, sizeof(events->tlv));
	if (got) {
		value = got->value;
		freq  = got->freq;
	}
	kfree(events);

	if (got)
		pos += snprintf(buf, len, "%d %d %d\n", value, freq,
			!!(le16_to_cpu(events->events) & event_mask));

	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);

	free_page(addr);
	return res;
}


static ssize_t lbs_threshold_write(
	u16 tlv_type, u16 event_mask,
	struct file *file,
	const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	int value, freq, curr_mask, new_mask;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;
	struct cmd_ds_802_11_subscribe_event *events;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	res = sscanf(buf, "%d %d %d", &value, &freq, &new_mask);
	if (res != 3) {
		res = -EFAULT;
		goto out_unlock;
	}
	curr_mask = lbs_get_events_bitmap(priv);

	if (new_mask)
		new_mask = curr_mask | event_mask;
	else
		new_mask = curr_mask & ~event_mask;

	/* Now everything is set and we can send stuff down to the firmware */
	events = kzalloc(
		sizeof(struct cmd_ds_802_11_subscribe_event),
		GFP_KERNEL);
	if (events) {
		struct mrvlietypes_thresholds *tlv =
			(struct mrvlietypes_thresholds *) events->tlv;
		events->action = cpu_to_le16(CMD_ACT_SET);
		events->events = cpu_to_le16(new_mask);
		tlv->header.type = cpu_to_le16(tlv_type);
		tlv->header.len = cpu_to_le16(
			sizeof(struct mrvlietypes_thresholds) -
			sizeof(struct mrvlietypesheader));
		tlv->value = value;
		if (tlv_type != TLV_TYPE_BCNMISS)
			tlv->freq = freq;
		lbs_prepare_and_send_command(priv,
			CMD_802_11_SUBSCRIBE_EVENT, CMD_ACT_SET,
			CMD_OPTION_WAITFORRSP, 0, events);
		kfree(events);
	}

	res = count;
out_unlock:
	free_page(addr);
	return res;
}


static ssize_t lbs_lowrssi_read(
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_read(TLV_TYPE_RSSI_LOW, CMD_SUBSCRIBE_RSSI_LOW,
		file, userbuf, count, ppos);
}


static ssize_t lbs_lowrssi_write(
	struct file *file, const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_write(TLV_TYPE_RSSI_LOW, CMD_SUBSCRIBE_RSSI_LOW,
		file, userbuf, count, ppos);
}


static ssize_t lbs_lowsnr_read(
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_read(TLV_TYPE_SNR_LOW, CMD_SUBSCRIBE_SNR_LOW,
		file, userbuf, count, ppos);
}


static ssize_t lbs_lowsnr_write(
	struct file *file, const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_write(TLV_TYPE_SNR_LOW, CMD_SUBSCRIBE_SNR_LOW,
		file, userbuf, count, ppos);
}


static ssize_t lbs_failcount_read(
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_read(TLV_TYPE_FAILCOUNT, CMD_SUBSCRIBE_FAILCOUNT,
		file, userbuf, count, ppos);
}


static ssize_t lbs_failcount_write(
	struct file *file, const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_write(TLV_TYPE_FAILCOUNT, CMD_SUBSCRIBE_FAILCOUNT,
		file, userbuf, count, ppos);
}


static ssize_t lbs_highrssi_read(
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_read(TLV_TYPE_RSSI_HIGH, CMD_SUBSCRIBE_RSSI_HIGH,
		file, userbuf, count, ppos);
}


static ssize_t lbs_highrssi_write(
	struct file *file, const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_write(TLV_TYPE_RSSI_HIGH, CMD_SUBSCRIBE_RSSI_HIGH,
		file, userbuf, count, ppos);
}


static ssize_t lbs_highsnr_read(
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_read(TLV_TYPE_SNR_HIGH, CMD_SUBSCRIBE_SNR_HIGH,
		file, userbuf, count, ppos);
}


static ssize_t lbs_highsnr_write(
	struct file *file, const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_write(TLV_TYPE_SNR_HIGH, CMD_SUBSCRIBE_SNR_HIGH,
		file, userbuf, count, ppos);
}

static ssize_t lbs_bcnmiss_read(
	struct file *file, char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_read(TLV_TYPE_BCNMISS, CMD_SUBSCRIBE_BCNMISS,
		file, userbuf, count, ppos);
}


static ssize_t lbs_bcnmiss_write(
	struct file *file, const char __user *userbuf,
	size_t count, loff_t *ppos)
{
	return lbs_threshold_write(TLV_TYPE_BCNMISS, CMD_SUBSCRIBE_BCNMISS,
		file, userbuf, count, ppos);
}








static ssize_t lbs_rdmac_read(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	struct lbs_adapter *adapter = priv->adapter;
	struct lbs_offset_value offval;
	ssize_t pos = 0;
	int ret;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	offval.offset = priv->mac_offset;
	offval.value = 0;

	ret = lbs_prepare_and_send_command(priv,
				CMD_MAC_REG_ACCESS, 0,
				CMD_OPTION_WAITFORRSP, 0, &offval);
	mdelay(10);
	pos += snprintf(buf+pos, len-pos, "MAC[0x%x] = 0x%08x\n",
				priv->mac_offset, adapter->offsetvalue.value);

	ret = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	free_page(addr);
	return ret;
}

static ssize_t lbs_rdmac_write(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	priv->mac_offset = simple_strtoul((char *)buf, NULL, 16);
	res = count;
out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_wrmac_write(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{

	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	u32 offset, value;
	struct lbs_offset_value offval;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	res = sscanf(buf, "%x %x", &offset, &value);
	if (res != 2) {
		res = -EFAULT;
		goto out_unlock;
	}

	offval.offset = offset;
	offval.value = value;
	res = lbs_prepare_and_send_command(priv,
				CMD_MAC_REG_ACCESS, 1,
				CMD_OPTION_WAITFORRSP, 0, &offval);
	mdelay(10);

	res = count;
out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_rdbbp_read(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	struct lbs_adapter *adapter = priv->adapter;
	struct lbs_offset_value offval;
	ssize_t pos = 0;
	int ret;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	offval.offset = priv->bbp_offset;
	offval.value = 0;

	ret = lbs_prepare_and_send_command(priv,
				CMD_BBP_REG_ACCESS, 0,
				CMD_OPTION_WAITFORRSP, 0, &offval);
	mdelay(10);
	pos += snprintf(buf+pos, len-pos, "BBP[0x%x] = 0x%08x\n",
				priv->bbp_offset, adapter->offsetvalue.value);

	ret = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	free_page(addr);

	return ret;
}

static ssize_t lbs_rdbbp_write(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	priv->bbp_offset = simple_strtoul((char *)buf, NULL, 16);
	res = count;
out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_wrbbp_write(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{

	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	u32 offset, value;
	struct lbs_offset_value offval;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	res = sscanf(buf, "%x %x", &offset, &value);
	if (res != 2) {
		res = -EFAULT;
		goto out_unlock;
	}

	offval.offset = offset;
	offval.value = value;
	res = lbs_prepare_and_send_command(priv,
				CMD_BBP_REG_ACCESS, 1,
				CMD_OPTION_WAITFORRSP, 0, &offval);
	mdelay(10);

	res = count;
out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_rdrf_read(struct file *file, char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	struct lbs_adapter *adapter = priv->adapter;
	struct lbs_offset_value offval;
	ssize_t pos = 0;
	int ret;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	offval.offset = priv->rf_offset;
	offval.value = 0;

	ret = lbs_prepare_and_send_command(priv,
				CMD_RF_REG_ACCESS, 0,
				CMD_OPTION_WAITFORRSP, 0, &offval);
	mdelay(10);
	pos += snprintf(buf+pos, len-pos, "RF[0x%x] = 0x%08x\n",
				priv->rf_offset, adapter->offsetvalue.value);

	ret = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	free_page(addr);

	return ret;
}

static ssize_t lbs_rdrf_write(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{
	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	priv->rf_offset = simple_strtoul((char *)buf, NULL, 16);
	res = count;
out_unlock:
	free_page(addr);
	return res;
}

static ssize_t lbs_wrrf_write(struct file *file,
				    const char __user *userbuf,
				    size_t count, loff_t *ppos)
{

	struct lbs_private *priv = file->private_data;
	ssize_t res, buf_size;
	u32 offset, value;
	struct lbs_offset_value offval;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	buf_size = min(count, len - 1);
	if (copy_from_user(buf, userbuf, buf_size)) {
		res = -EFAULT;
		goto out_unlock;
	}
	res = sscanf(buf, "%x %x", &offset, &value);
	if (res != 2) {
		res = -EFAULT;
		goto out_unlock;
	}

	offval.offset = offset;
	offval.value = value;
	res = lbs_prepare_and_send_command(priv,
				CMD_RF_REG_ACCESS, 1,
				CMD_OPTION_WAITFORRSP, 0, &offval);
	mdelay(10);

	res = count;
out_unlock:
	free_page(addr);
	return res;
}

#define FOPS(fread, fwrite) { \
	.owner = THIS_MODULE, \
	.open = open_file_generic, \
	.read = (fread), \
	.write = (fwrite), \
}

struct lbs_debugfs_files {
	char *name;
	int perm;
	struct file_operations fops;
};

static struct lbs_debugfs_files debugfs_files[] = {
	{ "info", 0444, FOPS(lbs_dev_info, write_file_dummy), },
	{ "getscantable", 0444, FOPS(lbs_getscantable,
					write_file_dummy), },
	{ "sleepparams", 0644, FOPS(lbs_sleepparams_read,
				lbs_sleepparams_write), },
	{ "extscan", 0600, FOPS(NULL, lbs_extscan), },
	{ "setuserscan", 0600, FOPS(NULL, lbs_setuserscan), },
};

static struct lbs_debugfs_files debugfs_events_files[] = {
	{"low_rssi", 0644, FOPS(lbs_lowrssi_read,
				lbs_lowrssi_write), },
	{"low_snr", 0644, FOPS(lbs_lowsnr_read,
				lbs_lowsnr_write), },
	{"failure_count", 0644, FOPS(lbs_failcount_read,
				lbs_failcount_write), },
	{"beacon_missed", 0644, FOPS(lbs_bcnmiss_read,
				lbs_bcnmiss_write), },
	{"high_rssi", 0644, FOPS(lbs_highrssi_read,
				lbs_highrssi_write), },
	{"high_snr", 0644, FOPS(lbs_highsnr_read,
				lbs_highsnr_write), },
};

static struct lbs_debugfs_files debugfs_regs_files[] = {
	{"rdmac", 0644, FOPS(lbs_rdmac_read, lbs_rdmac_write), },
	{"wrmac", 0600, FOPS(NULL, lbs_wrmac_write), },
	{"rdbbp", 0644, FOPS(lbs_rdbbp_read, lbs_rdbbp_write), },
	{"wrbbp", 0600, FOPS(NULL, lbs_wrbbp_write), },
	{"rdrf", 0644, FOPS(lbs_rdrf_read, lbs_rdrf_write), },
	{"wrrf", 0600, FOPS(NULL, lbs_wrrf_write), },
};

void lbs_debugfs_init(void)
{
	if (!lbs_dir)
		lbs_dir = debugfs_create_dir("lbs_wireless", NULL);

	return;
}

void lbs_debugfs_remove(void)
{
	if (lbs_dir)
		 debugfs_remove(lbs_dir);
	return;
}

void lbs_debugfs_init_one(struct lbs_private *priv, struct net_device *dev)
{
	int i;
	struct lbs_debugfs_files *files;
	if (!lbs_dir)
		goto exit;

	priv->debugfs_dir = debugfs_create_dir(dev->name, lbs_dir);
	if (!priv->debugfs_dir)
		goto exit;

	for (i=0; i<ARRAY_SIZE(debugfs_files); i++) {
		files = &debugfs_files[i];
		priv->debugfs_files[i] = debugfs_create_file(files->name,
							     files->perm,
							     priv->debugfs_dir,
							     priv,
							     &files->fops);
	}

	priv->events_dir = debugfs_create_dir("subscribed_events", priv->debugfs_dir);
	if (!priv->events_dir)
		goto exit;

	for (i=0; i<ARRAY_SIZE(debugfs_events_files); i++) {
		files = &debugfs_events_files[i];
		priv->debugfs_events_files[i] = debugfs_create_file(files->name,
							     files->perm,
							     priv->events_dir,
							     priv,
							     &files->fops);
	}

	priv->regs_dir = debugfs_create_dir("registers", priv->debugfs_dir);
	if (!priv->regs_dir)
		goto exit;

	for (i=0; i<ARRAY_SIZE(debugfs_regs_files); i++) {
		files = &debugfs_regs_files[i];
		priv->debugfs_regs_files[i] = debugfs_create_file(files->name,
							     files->perm,
							     priv->regs_dir,
							     priv,
							     &files->fops);
	}

#ifdef PROC_DEBUG
	lbs_debug_init(priv, dev);
#endif
exit:
	return;
}

void lbs_debugfs_remove_one(struct lbs_private *priv)
{
	int i;

	for(i=0; i<ARRAY_SIZE(debugfs_regs_files); i++)
		debugfs_remove(priv->debugfs_regs_files[i]);

	debugfs_remove(priv->regs_dir);

	for(i=0; i<ARRAY_SIZE(debugfs_events_files); i++)
		debugfs_remove(priv->debugfs_events_files[i]);

	debugfs_remove(priv->events_dir);
#ifdef PROC_DEBUG
	debugfs_remove(priv->debugfs_debug);
#endif
	for(i=0; i<ARRAY_SIZE(debugfs_files); i++)
		debugfs_remove(priv->debugfs_files[i]);
	debugfs_remove(priv->debugfs_dir);
}



/* debug entry */

#ifdef PROC_DEBUG

#define item_size(n)	(FIELD_SIZEOF(struct lbs_adapter, n))
#define item_addr(n)	(offsetof(struct lbs_adapter, n))


struct debug_data {
	char name[32];
	u32 size;
	size_t addr;
};

/* To debug any member of struct lbs_adapter, simply add one line here.
 */
static struct debug_data items[] = {
	{"intcounter", item_size(intcounter), item_addr(intcounter)},
	{"psmode", item_size(psmode), item_addr(psmode)},
	{"psstate", item_size(psstate), item_addr(psstate)},
};

static int num_of_items = ARRAY_SIZE(items);

/**
 *  @brief proc read function
 *
 *  @param page	   pointer to buffer
 *  @param s       read data starting position
 *  @param off     offset
 *  @param cnt     counter
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static ssize_t lbs_debugfs_read(struct file *file, char __user *userbuf,
			size_t count, loff_t *ppos)
{
	int val = 0;
	size_t pos = 0;
	ssize_t res;
	char *p;
	int i;
	struct debug_data *d;
	unsigned long addr = get_zeroed_page(GFP_KERNEL);
	char *buf = (char *)addr;

	p = buf;

	d = (struct debug_data *)file->private_data;

	for (i = 0; i < num_of_items; i++) {
		if (d[i].size == 1)
			val = *((u8 *) d[i].addr);
		else if (d[i].size == 2)
			val = *((u16 *) d[i].addr);
		else if (d[i].size == 4)
			val = *((u32 *) d[i].addr);
		else if (d[i].size == 8)
			val = *((u64 *) d[i].addr);

		pos += sprintf(p + pos, "%s=%d\n", d[i].name, val);
	}

	res = simple_read_from_buffer(userbuf, count, ppos, p, pos);

	free_page(addr);
	return res;
}

/**
 *  @brief proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static ssize_t lbs_debugfs_write(struct file *f, const char __user *buf,
			    size_t cnt, loff_t *ppos)
{
	int r, i;
	char *pdata;
	char *p;
	char *p0;
	char *p1;
	char *p2;
	struct debug_data *d = (struct debug_data *)f->private_data;

	pdata = kmalloc(cnt, GFP_KERNEL);
	if (pdata == NULL)
		return 0;

	if (copy_from_user(pdata, buf, cnt)) {
		lbs_deb_debugfs("Copy from user failed\n");
		kfree(pdata);
		return 0;
	}

	p0 = pdata;
	for (i = 0; i < num_of_items; i++) {
		do {
			p = strstr(p0, d[i].name);
			if (p == NULL)
				break;
			p1 = strchr(p, '\n');
			if (p1 == NULL)
				break;
			p0 = p1++;
			p2 = strchr(p, '=');
			if (!p2)
				break;
			p2++;
			r = simple_strtoul(p2, NULL, 0);
			if (d[i].size == 1)
				*((u8 *) d[i].addr) = (u8) r;
			else if (d[i].size == 2)
				*((u16 *) d[i].addr) = (u16) r;
			else if (d[i].size == 4)
				*((u32 *) d[i].addr) = (u32) r;
			else if (d[i].size == 8)
				*((u64 *) d[i].addr) = (u64) r;
			break;
		} while (1);
	}
	kfree(pdata);

	return (ssize_t)cnt;
}

static struct file_operations lbs_debug_fops = {
	.owner = THIS_MODULE,
	.open = open_file_generic,
	.write = lbs_debugfs_write,
	.read = lbs_debugfs_read,
};

/**
 *  @brief create debug proc file
 *
 *  @param priv	   pointer struct lbs_private
 *  @param dev     pointer net_device
 *  @return 	   N/A
 */
static void lbs_debug_init(struct lbs_private *priv, struct net_device *dev)
{
	int i;

	if (!priv->debugfs_dir)
		return;

	for (i = 0; i < num_of_items; i++)
		items[i].addr += (size_t) priv->adapter;

	priv->debugfs_debug = debugfs_create_file("debug", 0644,
						  priv->debugfs_dir, &items[0],
						  &lbs_debug_fops);
}
#endif
