/*
    driver for LSI L64781 COFDM demodulator

    Copyright (C) 2001 Holger Waechtler for Convergence Integrated Media GmbH
                       Marko Kohtala <marko.kohtala@luukku.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "dvb_frontend.h"
#include "l64781.h"


struct l64781_state {
	struct i2c_adapter* i2c;
	struct dvb_frontend_ops ops;
	const struct l64781_config* config;
	struct dvb_frontend frontend;

	/* private demodulator data */
	int first:1;
};

#define dprintk(args...) \
	do { \
		if (debug) printk(KERN_DEBUG "l64781: " args); \
	} while (0)

static int debug;

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Turn on/off frontend debugging (default:off).");


static int l64781_writereg (struct l64781_state* state, u8 reg, u8 data)
{
	int ret;
	u8 buf [] = { reg, data };
	struct i2c_msg msg = { .addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2 };

	if ((ret = i2c_transfer(state->i2c, &msg, 1)) != 1)
		dprintk ("%s: write_reg error (reg == %02x) = %02x!\n",
			 __FUNCTION__, reg, ret);

	return (ret != 1) ? -1 : 0;
}

static int l64781_readreg (struct l64781_state* state, u8 reg)
{
	int ret;
	u8 b0 [] = { reg };
	u8 b1 [] = { 0 };
	struct i2c_msg msg [] = { { .addr = state->config->demod_address, .flags = 0, .buf = b0, .len = 1 },
			   { .addr = state->config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1 } };

	ret = i2c_transfer(state->i2c, msg, 2);

	if (ret != 2) return ret;

	return b1[0];
}

static void apply_tps (struct l64781_state* state)
{
	l64781_writereg (state, 0x2a, 0x00);
	l64781_writereg (state, 0x2a, 0x01);

	/* This here is a little bit questionable because it enables
	   the automatic update of TPS registers. I think we'd need to
	   handle the IRQ from FE to update some other registers as
	   well, or at least implement some magic to tuning to correct
	   to the TPS received from transmission. */
	l64781_writereg (state, 0x2a, 0x02);
}


static void reset_afc (struct l64781_state* state)
{
	/* Set AFC stall for the AFC_INIT_FRQ setting, TIM_STALL for
	   timing offset */
	l64781_writereg (state, 0x07, 0x9e); /* stall AFC */
	l64781_writereg (state, 0x08, 0);    /* AFC INIT FREQ */
	l64781_writereg (state, 0x09, 0);
	l64781_writereg (state, 0x0a, 0);
	l64781_writereg (state, 0x07, 0x8e);
	l64781_writereg (state, 0x0e, 0);    /* AGC gain to zero in beginning */
	l64781_writereg (state, 0x11, 0x80); /* stall TIM */
	l64781_writereg (state, 0x10, 0);    /* TIM_OFFSET_LSB */
	l64781_writereg (state, 0x12, 0);
	l64781_writereg (state, 0x13, 0);
	l64781_writereg (state, 0x11, 0x00);
}

static int reset_and_configure (struct l64781_state* state)
{
	u8 buf [] = { 0x06 };
	struct i2c_msg msg = { .addr = 0x00, .flags = 0, .buf = buf, .len = 1 };
	// NOTE: this is correct in writing to address 0x00

	return (i2c_transfer(state->i2c, &msg, 1) == 1) ? 0 : -ENODEV;
}

static int apply_frontend_param (struct dvb_frontend* fe, struct dvb_frontend_parameters *param)
{
	struct l64781_state* state = fe->demodulator_priv;
	/* The coderates for FEC_NONE, FEC_4_5 and FEC_FEC_6_7 are arbitrary */
	static const u8 fec_tab[] = { 7, 0, 1, 2, 9, 3, 10, 4 };
	/* QPSK, QAM_16, QAM_64 */
	static const u8 qam_tab [] = { 2, 4, 0, 6 };
	static const u8 bw_tab [] = { 8, 7, 6 };  /* 8Mhz, 7MHz, 6MHz */
	static const u8 guard_tab [] = { 1, 2, 4, 8 };
	/* The Grundig 29504-401.04 Tuner comes with 18.432MHz crystal. */
	static const u32 ppm = 8000;
	struct dvb_ofdm_parameters *p = &param->u.ofdm;
	u32 ddfs_offset_fixed;
/*	u32 ddfs_offset_variable = 0x6000-((1000000UL+ppm)/ */
/*			bw_tab[p->bandWidth]<<10)/15625; */
	u32 init_freq;
	u32 spi_bias;
	u8 val0x04;
	u8 val0x05;
	u8 val0x06;
	int bw = p->bandwidth - BANDWIDTH_8_MHZ;

	state->config->pll_set(fe, param);

	if (param->inversion != INVERSION_ON &&
	    param->inversion != INVERSION_OFF)
		return -EINVAL;

	if (bw < 0 || bw > 2)
		return -EINVAL;

	if (p->code_rate_HP != FEC_1_2 && p->code_rate_HP != FEC_2_3 &&
	    p->code_rate_HP != FEC_3_4 && p->code_rate_HP != FEC_5_6 &&
	    p->code_rate_HP != FEC_7_8)
		return -EINVAL;

	if (p->hierarchy_information != HIERARCHY_NONE &&
	    (p->code_rate_LP != FEC_1_2 && p->code_rate_LP != FEC_2_3 &&
	     p->code_rate_LP != FEC_3_4 && p->code_rate_LP != FEC_5_6 &&
	     p->code_rate_LP != FEC_7_8))
		return -EINVAL;

	if (p->constellation != QPSK && p->constellation != QAM_16 &&
	    p->constellation != QAM_64)
		return -EINVAL;

	if (p->transmission_mode != TRANSMISSION_MODE_2K &&
	    p->transmission_mode != TRANSMISSION_MODE_8K)
		return -EINVAL;

	if (p->guard_interval < GUARD_INTERVAL_1_32 ||
	    p->guard_interval > GUARD_INTERVAL_1_4)
		return -EINVAL;

	if (p->hierarchy_information < HIERARCHY_NONE ||
	    p->hierarchy_information > HIERARCHY_4)
		return -EINVAL;

	ddfs_offset_fixed = 0x4000-(ppm<<16)/bw_tab[p->bandwidth]/1000000;

	/* This works up to 20000 ppm, it overflows if too large ppm! */
	init_freq = (((8UL<<25) + (8UL<<19) / 25*ppm / (15625/25)) /
			bw_tab[p->bandwidth] & 0xFFFFFF);

	/* SPI bias calculation is slightly modified to fit in 32bit */
	/* will work for high ppm only... */
	spi_bias = 378 * (1 << 10);
	spi_bias *= 16;
	spi_bias *= bw_tab[p->bandwidth];
	spi_bias *= qam_tab[p->constellation];
	spi_bias /= p->code_rate_HP + 1;
	spi_bias /= (guard_tab[p->guard_interval] + 32);
	spi_bias *= 1000ULL;
	spi_bias /= 1000ULL + ppm/1000;
	spi_bias *= p->code_rate_HP;

	val0x04 = (p->transmission_mode << 2) | p->guard_interval;
	val0x05 = fec_tab[p->code_rate_HP];

	if (p->hierarchy_information != HIERARCHY_NONE)
		val0x05 |= (p->code_rate_LP - FEC_1_2) << 3;

	val0x06 = (p->hierarchy_information << 2) | p->constellation;

	l64781_writereg (state, 0x04, val0x04);
	l64781_writereg (state, 0x05, val0x05);
	l64781_writereg (state, 0x06, val0x06);

	reset_afc (state);

	/* Technical manual section 2.6.1, TIM_IIR_GAIN optimal values */
	l64781_writereg (state, 0x15,
			 p->transmission_mode == TRANSMISSION_MODE_2K ? 1 : 3);
	l64781_writereg (state, 0x16, init_freq & 0xff);
	l64781_writereg (state, 0x17, (init_freq >> 8) & 0xff);
	l64781_writereg (state, 0x18, (init_freq >> 16) & 0xff);

	l64781_writereg (state, 0x1b, spi_bias & 0xff);
	l64781_writereg (state, 0x1c, (spi_bias >> 8) & 0xff);
	l64781_writereg (state, 0x1d, ((spi_bias >> 16) & 0x7f) |
		(param->inversion == INVERSION_ON ? 0x80 : 0x00));

	l64781_writereg (state, 0x22, ddfs_offset_fixed & 0xff);
	l64781_writereg (state, 0x23, (ddfs_offset_fixed >> 8) & 0x3f);

	l64781_readreg (state, 0x00);  /*  clear interrupt registers... */
	l64781_readreg (state, 0x01);  /*  dto. */

	apply_tps (state);

	return 0;
}

static int get_frontend(struct dvb_frontend* fe, struct dvb_frontend_parameters* param)
{
	struct l64781_state* state = fe->demodulator_priv;
	int tmp;


	tmp = l64781_readreg(state, 0x04);
	switch(tmp & 3) {
	case 0:
		param->u.ofdm.guard_interval = GUARD_INTERVAL_1_32;
		break;
	case 1:
		param->u.ofdm.guard_interval = GUARD_INTERVAL_1_16;
		break;
	case 2:
		param->u.ofdm.guard_interval = GUARD_INTERVAL_1_8;
		break;
	case 3:
		param->u.ofdm.guard_interval = GUARD_INTERVAL_1_4;
		break;
	}
	switch((tmp >> 2) & 3) {
	case 0:
		param->u.ofdm.transmission_mode = TRANSMISSION_MODE_2K;
		break;
	case 1:
		param->u.ofdm.transmission_mode = TRANSMISSION_MODE_8K;
		break;
	default:
		printk("Unexpected value for transmission_mode\n");
	}



	tmp = l64781_readreg(state, 0x05);
	switch(tmp & 7) {
	case 0:
		param->u.ofdm.code_rate_HP = FEC_1_2;
		break;
	case 1:
		param->u.ofdm.code_rate_HP = FEC_2_3;
		break;
	case 2:
		param->u.ofdm.code_rate_HP = FEC_3_4;
		break;
	case 3:
		param->u.ofdm.code_rate_HP = FEC_5_6;
		break;
	case 4:
		param->u.ofdm.code_rate_HP = FEC_7_8;
		break;
	default:
		printk("Unexpected value for code_rate_HP\n");
	}
	switch((tmp >> 3) & 7) {
	case 0:
		param->u.ofdm.code_rate_LP = FEC_1_2;
		break;
	case 1:
		param->u.ofdm.code_rate_LP = FEC_2_3;
		break;
	case 2:
		param->u.ofdm.code_rate_LP = FEC_3_4;
		break;
	case 3:
		param->u.ofdm.code_rate_LP = FEC_5_6;
		break;
	case 4:
		param->u.ofdm.code_rate_LP = FEC_7_8;
		break;
	default:
		printk("Unexpected value for code_rate_LP\n");
	}


	tmp = l64781_readreg(state, 0x06);
	switch(tmp & 3) {
	case 0:
		param->u.ofdm.constellation = QPSK;
		break;
	case 1:
		param->u.ofdm.constellation = QAM_16;
		break;
	case 2:
		param->u.ofdm.constellation = QAM_64;
		break;
	default:
		printk("Unexpected value for constellation\n");
	}
	switch((tmp >> 2) & 7) {
	case 0:
		param->u.ofdm.hierarchy_information = HIERARCHY_NONE;
		break;
	case 1:
		param->u.ofdm.hierarchy_information = HIERARCHY_1;
		break;
	case 2:
		param->u.ofdm.hierarchy_information = HIERARCHY_2;
		break;
	case 3:
		param->u.ofdm.hierarchy_information = HIERARCHY_4;
		break;
	default:
		printk("Unexpected value for hierarchy\n");
	}


	tmp = l64781_readreg (state, 0x1d);
	param->inversion = (tmp & 0x80) ? INVERSION_ON : INVERSION_OFF;

	tmp = (int) (l64781_readreg (state, 0x08) |
		     (l64781_readreg (state, 0x09) << 8) |
		     (l64781_readreg (state, 0x0a) << 16));
	param->frequency += tmp;

	return 0;
}

static int l64781_read_status(struct dvb_frontend* fe, fe_status_t* status)
{
	struct l64781_state* state = fe->demodulator_priv;
	int sync = l64781_readreg (state, 0x32);
	int gain = l64781_readreg (state, 0x0e);

	l64781_readreg (state, 0x00);  /*  clear interrupt registers... */
	l64781_readreg (state, 0x01);  /*  dto. */

	*status = 0;

	if (gain > 5)
		*status |= FE_HAS_SIGNAL;

	if (sync & 0x02) /* VCXO locked, this criteria should be ok */
		*status |= FE_HAS_CARRIER;

	if (sync & 0x20)
		*status |= FE_HAS_VITERBI;

	if (sync & 0x40)
		*status |= FE_HAS_SYNC;

	if (sync == 0x7f)
		*status |= FE_HAS_LOCK;

	return 0;
}

static int l64781_read_ber(struct dvb_frontend* fe, u32* ber)
{
	struct l64781_state* state = fe->demodulator_priv;

	/*   XXX FIXME: set up counting period (reg 0x26...0x28)
	 */
	*ber = l64781_readreg (state, 0x39)
	    | (l64781_readreg (state, 0x3a) << 8);

	return 0;
}

static int l64781_read_signal_strength(struct dvb_frontend* fe, u16* signal_strength)
{
	struct l64781_state* state = fe->demodulator_priv;

	u8 gain = l64781_readreg (state, 0x0e);
	*signal_strength = (gain << 8) | gain;

	return 0;
}

static int l64781_read_snr(struct dvb_frontend* fe, u16* snr)
{
	struct l64781_state* state = fe->demodulator_priv;

	u8 avg_quality = 0xff - l64781_readreg (state, 0x33);
	*snr = (avg_quality << 8) | avg_quality; /* not exact, but...*/

	return 0;
}

static int l64781_read_ucblocks(struct dvb_frontend* fe, u32* ucblocks)
{
	struct l64781_state* state = fe->demodulator_priv;

	*ucblocks = l64781_readreg (state, 0x37)
	   | (l64781_readreg (state, 0x38) << 8);

	return 0;
}

static int l64781_sleep(struct dvb_frontend* fe)
{
	struct l64781_state* state = fe->demodulator_priv;

	/* Power down */
	return l64781_writereg (state, 0x3e, 0x5a);
}

static int l64781_init(struct dvb_frontend* fe)
{
	struct l64781_state* state = fe->demodulator_priv;

        reset_and_configure (state);

	/* Power up */
	l64781_writereg (state, 0x3e, 0xa5);

	/* Reset hard */
	l64781_writereg (state, 0x2a, 0x04);
	l64781_writereg (state, 0x2a, 0x00);

	/* Set tuner specific things */
	/* AFC_POL, set also in reset_afc */
	l64781_writereg (state, 0x07, 0x8e);

	/* Use internal ADC */
	l64781_writereg (state, 0x0b, 0x81);

	/* AGC loop gain, and polarity is positive */
	l64781_writereg (state, 0x0c, 0x84);

	/* Internal ADC outputs two's complement */
	l64781_writereg (state, 0x0d, 0x8c);

	/* With ppm=8000, it seems the DTR_SENSITIVITY will result in
           value of 2 with all possible bandwidths and guard
           intervals, which is the initial value anyway. */
        /*l64781_writereg (state, 0x19, 0x92);*/

	/* Everything is two's complement, soft bit and CSI_OUT too */
	l64781_writereg (state, 0x1e, 0x09);

	if (state->config->pll_init) state->config->pll_init(fe);

	/* delay a bit after first init attempt */
	if (state->first) {
		state->first = 0;
		msleep(200);
	}

	return 0;
}

static int l64781_get_tune_settings(struct dvb_frontend* fe, struct dvb_frontend_tune_settings* fesettings)
{
        fesettings->min_delay_ms = 200;
        fesettings->step_size = 166667;
        fesettings->max_drift = 166667*2;
        return 0;
}

static void l64781_release(struct dvb_frontend* fe)
{
	struct l64781_state* state = fe->demodulator_priv;
	kfree(state);
}

static struct dvb_frontend_ops l64781_ops;

struct dvb_frontend* l64781_attach(const struct l64781_config* config,
				   struct i2c_adapter* i2c)
{
	struct l64781_state* state = NULL;
	int reg0x3e = -1;
	u8 b0 [] = { 0x1a };
	u8 b1 [] = { 0x00 };
	struct i2c_msg msg [] = { { .addr = config->demod_address, .flags = 0, .buf = b0, .len = 1 },
			   { .addr = config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1 } };

	/* allocate memory for the internal state */
	state = kmalloc(sizeof(struct l64781_state), GFP_KERNEL);
	if (state == NULL) goto error;

	/* setup the state */
	state->config = config;
	state->i2c = i2c;
	memcpy(&state->ops, &l64781_ops, sizeof(struct dvb_frontend_ops));
	state->first = 1;

	/**
	 *  the L64781 won't show up before we send the reset_and_configure()
	 *  broadcast. If nothing responds there is no L64781 on the bus...
	 */
	if (reset_and_configure(state) < 0) {
		dprintk("No response to reset and configure broadcast...\n");
		goto error;
	}

	/* The chip always responds to reads */
	if (i2c_transfer(state->i2c, msg, 2) != 2) {
	        dprintk("No response to read on I2C bus\n");
		goto error;
	}

	/* Save current register contents for bailout */
	reg0x3e = l64781_readreg(state, 0x3e);

	/* Reading the POWER_DOWN register always returns 0 */
	if (reg0x3e != 0) {
	        dprintk("Device doesn't look like L64781\n");
		goto error;
	}

	/* Turn the chip off */
	l64781_writereg (state, 0x3e, 0x5a);

	/* Responds to all reads with 0 */
	if (l64781_readreg(state, 0x1a) != 0) {
	        dprintk("Read 1 returned unexpcted value\n");
		goto error;
	}

	/* Turn the chip on */
	l64781_writereg (state, 0x3e, 0xa5);

	/* Responds with register default value */
	if (l64781_readreg(state, 0x1a) != 0xa1) {
	        dprintk("Read 2 returned unexpcted value\n");
		goto error;
	}

	/* create dvb_frontend */
	state->frontend.ops = &state->ops;
	state->frontend.demodulator_priv = state;
	return &state->frontend;

error:
	if (reg0x3e >= 0) l64781_writereg (state, 0x3e, reg0x3e);  /* restore reg 0x3e */
	kfree(state);
	return NULL;
}

static struct dvb_frontend_ops l64781_ops = {

	.info = {
		.name = "LSI L64781 DVB-T",
		.type = FE_OFDM,
	/*	.frequency_min = ???,*/
	/*	.frequency_max = ???,*/
		.frequency_stepsize = 166666,
	/*      .frequency_tolerance = ???,*/
	/*      .symbol_rate_tolerance = ???,*/
		.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
		      FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 |
		      FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_64 |
		      FE_CAN_MUTE_TS
	},

	.release = l64781_release,

	.init = l64781_init,
	.sleep = l64781_sleep,

	.set_frontend = apply_frontend_param,
	.get_frontend = get_frontend,
	.get_tune_settings = l64781_get_tune_settings,

	.read_status = l64781_read_status,
	.read_ber = l64781_read_ber,
	.read_signal_strength = l64781_read_signal_strength,
	.read_snr = l64781_read_snr,
	.read_ucblocks = l64781_read_ucblocks,
};

MODULE_DESCRIPTION("LSI L64781 DVB-T Demodulator driver");
MODULE_AUTHOR("Holger Waechtler, Marko Kohtala");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(l64781_attach);
