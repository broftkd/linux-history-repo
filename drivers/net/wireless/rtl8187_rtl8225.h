#ifndef RTL8187_RTL8225_H
#define RTL8187_RTL8225_H

#define RTL8225_ANAPARAM_ON	0xa0000a59
#define RTL8225_ANAPARAM2_ON	0x860c7312
#define RTL8225_ANAPARAM_OFF	0xa00beb59
#define RTL8225_ANAPARAM2_OFF	0x840dec11

void rtl8225_write(struct ieee80211_hw *, u8 addr, u16 data);
u16  rtl8225_read(struct ieee80211_hw *, u8 addr);

void rtl8225_rf_init(struct ieee80211_hw *);
void rtl8225z2_rf_init(struct ieee80211_hw *);
void rtl8225_rf_stop(struct ieee80211_hw *);
void rtl8225_rf_set_channel(struct ieee80211_hw *, int);


static inline void rtl8225_write_phy_ofdm(struct ieee80211_hw *dev,
					  u8 addr, u32 data)
{
	rtl8187_write_phy(dev, addr, data);
}

static inline void rtl8225_write_phy_cck(struct ieee80211_hw *dev,
					 u8 addr, u32 data)
{
	rtl8187_write_phy(dev, addr, data | 0x10000);
}

#endif /* RTL8187_RTL8225_H */
