/*
 * Driver for the ov9650 sensor
 *
 * Copyright (C) 2008 Erik Andrén
 * Copyright (C) 2007 Ilyes Gouta. Based on the m5603x Linux Driver Project.
 * Copyright (C) 2005 m5603x Linux Driver Project <m5602@x3ng.com.br>
 *
 * Portions of code to USB interface and ALi driver software,
 * Copyright (c) 2006 Willem Duinker
 * v4l2 interface modeled after the V4L2 driver
 * for SN9C10x PC Camera Controllers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 *
 */

#include "m5602_ov9650.h"

/* Vertically and horizontally flips the image if matched, needed for machines
   where the sensor is mounted upside down */
static
    const
	struct dmi_system_id ov9650_flip_dmi_table[] = {
	{
		.ident = "ASUS A6VC",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "ASUSTeK Computer Inc."),
			DMI_MATCH(DMI_PRODUCT_NAME, "A6VC")
		}
	},
	{
		.ident = "ASUS A6VM",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "ASUSTeK Computer Inc."),
			DMI_MATCH(DMI_PRODUCT_NAME, "A6VM")
		}
	},
	{
		.ident = "ASUS A6JC",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "ASUSTeK Computer Inc."),
			DMI_MATCH(DMI_PRODUCT_NAME, "A6JC")
		}
	},
	{
		.ident = "ASUS A6Ja",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "ASUSTeK Computer Inc."),
			DMI_MATCH(DMI_PRODUCT_NAME, "A6J")
		}
	},
	{
		.ident = "ASUS A6Kt",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "ASUSTeK Computer Inc."),
			DMI_MATCH(DMI_PRODUCT_NAME, "A6Kt")
		}
	},
	{
		.ident = "Alienware Aurora m9700",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "alienware"),
			DMI_MATCH(DMI_PRODUCT_NAME, "Aurora m9700")
		}
	},
	{ }
};

const static struct ctrl ov9650_ctrls[] = {
	{
		{
			.id		= V4L2_CID_EXPOSURE,
			.type		= V4L2_CTRL_TYPE_INTEGER,
			.name		= "exposure",
			.minimum	= 0x00,
			.maximum	= 0x1ff,
			.step		= 0x4,
			.default_value 	= EXPOSURE_DEFAULT,
			.flags         	= V4L2_CTRL_FLAG_SLIDER
		},
		.set = ov9650_set_exposure,
		.get = ov9650_get_exposure
	}, {
		{
			.id		= V4L2_CID_GAIN,
			.type		= V4L2_CTRL_TYPE_INTEGER,
			.name		= "gain",
			.minimum	= 0x00,
			.maximum	= 0x3ff,
			.step		= 0x1,
			.default_value	= GAIN_DEFAULT,
			.flags		= V4L2_CTRL_FLAG_SLIDER
		},
		.set = ov9650_set_gain,
		.get = ov9650_get_gain
	}, {
		{
			.type 		= V4L2_CTRL_TYPE_INTEGER,
			.name 		= "red balance",
			.minimum 	= 0x00,
			.maximum 	= 0xff,
			.step 		= 0x1,
			.default_value 	= RED_GAIN_DEFAULT,
			.flags         	= V4L2_CTRL_FLAG_SLIDER
		},
		.set = ov9650_set_red_balance,
		.get = ov9650_get_red_balance
	}, {
		{
			.type 		= V4L2_CTRL_TYPE_INTEGER,
			.name 		= "blue balance",
			.minimum 	= 0x00,
			.maximum 	= 0xff,
			.step 		= 0x1,
			.default_value 	= BLUE_GAIN_DEFAULT,
			.flags         	= V4L2_CTRL_FLAG_SLIDER
		},
		.set = ov9650_set_blue_balance,
		.get = ov9650_get_blue_balance
	}, {
		{
			.id 		= V4L2_CID_HFLIP,
			.type 		= V4L2_CTRL_TYPE_BOOLEAN,
			.name 		= "horizontal flip",
			.minimum 	= 0,
			.maximum 	= 1,
			.step 		= 1,
			.default_value 	= 0
		},
		.set = ov9650_set_hflip,
		.get = ov9650_get_hflip
	}, {
		{
			.id 		= V4L2_CID_VFLIP,
			.type 		= V4L2_CTRL_TYPE_BOOLEAN,
			.name 		= "vertical flip",
			.minimum 	= 0,
			.maximum 	= 1,
			.step 		= 1,
			.default_value 	= 0
		},
		.set = ov9650_set_vflip,
		.get = ov9650_get_vflip
	}, {
		{
			.id 		= V4L2_CID_AUTO_WHITE_BALANCE,
			.type 		= V4L2_CTRL_TYPE_BOOLEAN,
			.name 		= "auto white balance",
			.minimum 	= 0,
			.maximum 	= 1,
			.step 		= 1,
			.default_value 	= 1
		},
		.set = ov9650_set_auto_white_balance,
		.get = ov9650_get_auto_white_balance
	}, {
		{
			.id 		= V4L2_CID_AUTOGAIN,
			.type 		= V4L2_CTRL_TYPE_BOOLEAN,
			.name 		= "auto gain control",
			.minimum 	= 0,
			.maximum 	= 1,
			.step 		= 1,
			.default_value 	= 1
		},
		.set = ov9650_set_auto_gain,
		.get = ov9650_get_auto_gain
	}
};

static struct v4l2_pix_format ov9650_modes[] = {
	{
		176,
		144,
		V4L2_PIX_FMT_SBGGR8,
		V4L2_FIELD_NONE,
		.sizeimage =
			176 * 144,
		.bytesperline = 176,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.priv = 0
	}, {
		320,
		240,
		V4L2_PIX_FMT_SBGGR8,
		V4L2_FIELD_NONE,
		.sizeimage =
			320 * 240,
		.bytesperline = 320,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.priv = 0
	}, {
		352,
		288,
		V4L2_PIX_FMT_SBGGR8,
		V4L2_FIELD_NONE,
		.sizeimage =
			352 * 288,
		.bytesperline = 352,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.priv = 0
	}, {
		640,
		480,
		V4L2_PIX_FMT_SBGGR8,
		V4L2_FIELD_NONE,
		.sizeimage =
			640 * 480,
		.bytesperline = 640,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.priv = 0
	}
};

static void ov9650_dump_registers(struct sd *sd);

int ov9650_probe(struct sd *sd)
{
	u8 prod_id = 0, ver_id = 0, i;

	if (force_sensor) {
		if (force_sensor == OV9650_SENSOR) {
			info("Forcing an %s sensor", ov9650.name);
			goto sensor_found;
		}
		/* If we want to force another sensor,
		   don't try to probe this one */
		return -ENODEV;
	}

	info("Probing for an ov9650 sensor");

	/* Run the pre-init to actually probe the unit */
	for (i = 0; i < ARRAY_SIZE(preinit_ov9650); i++) {
		u8 data = preinit_ov9650[i][2];
		if (preinit_ov9650[i][0] == SENSOR)
			m5602_write_sensor(sd,
					    preinit_ov9650[i][1], &data, 1);
		else
			m5602_write_bridge(sd, preinit_ov9650[i][1], data);
	}

	if (m5602_read_sensor(sd, OV9650_PID, &prod_id, 1))
		return -ENODEV;

	if (m5602_read_sensor(sd, OV9650_VER, &ver_id, 1))
		return -ENODEV;

	if ((prod_id == 0x96) && (ver_id == 0x52)) {
		info("Detected an ov9650 sensor");
		goto sensor_found;
	}

	return -ENODEV;

sensor_found:
	sd->gspca_dev.cam.cam_mode = ov9650_modes;
	sd->gspca_dev.cam.nmodes = ARRAY_SIZE(ov9650_modes);
	sd->desc->ctrls = ov9650_ctrls;
	sd->desc->nctrls = ARRAY_SIZE(ov9650_ctrls);
	return 0;
}

int ov9650_init(struct sd *sd)
{
	int i, err = 0;
	u8 data;

	if (dump_sensor)
		ov9650_dump_registers(sd);

	for (i = 0; i < ARRAY_SIZE(init_ov9650) && !err; i++) {
		data = init_ov9650[i][2];
		if (init_ov9650[i][0] == SENSOR)
			err = m5602_write_sensor(sd, init_ov9650[i][1],
						  &data, 1);
		else
			err = m5602_write_bridge(sd, init_ov9650[i][1], data);
	}

	if (dmi_check_system(ov9650_flip_dmi_table) && !err) {
		info("vflip quirk active");
		data = 0x30;
		err = m5602_write_sensor(sd, OV9650_MVFP, &data, 1);
	}

	return err;
}

int ov9650_start(struct sd *sd)
{
	int i, err = 0;
	struct cam *cam = &sd->gspca_dev.cam;

	err = ov9650_init(sd);
	if (err < 0)
		return err;

	for (i = 0; i < ARRAY_SIZE(res_init_ov9650) && !err; i++) {
		if (res_init_ov9650[i][0] == BRIDGE)
			err = m5602_write_bridge(sd, res_init_ov9650[i][1],
				res_init_ov9650[i][2]);
		else if (res_init_ov9650[i][0] == SENSOR) {
			u8 data = res_init_ov9650[i][2];
			err = m5602_write_sensor(sd,
				res_init_ov9650[i][1], &data, 1);
		}
	}
	if (err < 0)
		return err;

	switch (cam->cam_mode[sd->gspca_dev.curr_mode].width) {
	case 640:
		PDEBUG(D_V4L2, "Configuring camera for VGA mode");

		for (i = 0; i < ARRAY_SIZE(VGA_ov9650) && !err; i++) {
			if (VGA_ov9650[i][0] == SENSOR) {
				u8 data = VGA_ov9650[i][2];

				err = m5602_write_sensor(sd,
					VGA_ov9650[i][1], &data, 1);
			} else {
				err = m5602_write_bridge(sd, VGA_ov9650[i][1],
						VGA_ov9650[i][2]);
			}
		}
		break;

	case 352:
		PDEBUG(D_V4L2, "Configuring camera for CIF mode");

		for (i = 0; i < ARRAY_SIZE(CIF_ov9650) && !err; i++) {
			if (CIF_ov9650[i][0] == SENSOR) {
				u8 data = CIF_ov9650[i][2];

				err = m5602_write_sensor(sd,
					CIF_ov9650[i][1], &data, 1);
			} else {
				err = m5602_write_bridge(sd, CIF_ov9650[i][1],
						CIF_ov9650[i][2]);
			}
		}
		break;

	case 320:
		PDEBUG(D_V4L2, "Configuring camera for QVGA mode");

		for (i = 0; i < ARRAY_SIZE(QVGA_ov9650) && !err; i++) {
			if (QVGA_ov9650[i][0] == SENSOR) {
				u8 data = QVGA_ov9650[i][2];

				err = m5602_write_sensor(sd,
					QVGA_ov9650[i][1], &data, 1);
			} else {
				err = m5602_write_bridge(sd, QVGA_ov9650[i][1],
						QVGA_ov9650[i][2]);
			}
		}
		break;

	case 176:
		PDEBUG(D_V4L2, "Configuring camera for QCIF mode");

		for (i = 0; i < ARRAY_SIZE(QCIF_ov9650) && !err; i++) {
			if (QCIF_ov9650[i][0] == SENSOR) {
				u8 data = QCIF_ov9650[i][2];
				err = m5602_write_sensor(sd,
					QCIF_ov9650[i][1], &data, 1);
			} else {
				err = m5602_write_bridge(sd, QCIF_ov9650[i][1],
						QCIF_ov9650[i][2]);
			}
		}
		break;

	}
	return err;
}

int ov9650_stop(struct sd *sd)
{
	u8 data = OV9650_SOFT_SLEEP | OV9650_OUTPUT_DRIVE_2X;
	return m5602_write_sensor(sd, OV9650_COM2, &data, 1);
}

int ov9650_power_down(struct sd *sd)
{
	int i, err = 0;
	for (i = 0; i < ARRAY_SIZE(power_down_ov9650) && !err; i++) {
		u8 data = power_down_ov9650[i][2];
		if (power_down_ov9650[i][0] == SENSOR)
			err = m5602_write_sensor(sd,
					    power_down_ov9650[i][1], &data, 1);
		else
			err = m5602_write_bridge(sd, power_down_ov9650[i][1],
						 data);
	}

	return err;
}

int ov9650_get_exposure(struct gspca_dev *gspca_dev, __s32 *val)
{
	struct sd *sd = (struct sd *) gspca_dev;
	u8 i2c_data;
	int err;

	err = m5602_read_sensor(sd, OV9650_AECH, &i2c_data, 1);
	if (err < 0)
		return err;
	*val |= (i2c_data << 2);

	err = m5602_read_sensor(sd, OV9650_AECHM, &i2c_data, 1);
	if (err < 0)
		return err;
	*val |= (i2c_data & 0x3f) << 10;

	PDEBUG(D_V4L2, "Read exposure %d", *val);

	return err;
}

int ov9650_set_exposure(struct gspca_dev *gspca_dev, __s32 val)
{
	struct sd *sd = (struct sd *) gspca_dev;
	u8 i2c_data;
	int err;

	PDEBUG(D_V4L2, "Set exposure to %d",
	       val & 0xffff);

	/* The 6 MSBs */
	i2c_data = (val >> 10) & 0x3f;
	err = m5602_write_sensor(sd, OV9650_AECHM,
				  &i2c_data, 1);
	if (err < 0)
		return err;

	/* The 8 middle bits */
	i2c_data = (val >> 2) & 0xff;
	err = m5602_write_sensor(sd, OV9650_AECH,
				  &i2c_data, 1);
	if (err < 0)
		return err;

	/* The 2 LSBs */
	i2c_data = val & 0x03;
	err = m5602_write_sensor(sd, OV9650_COM1, &i2c_data, 1);

	return err;
}

int ov9650_get_gain(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	m5602_read_sensor(sd, OV9650_VREF, &i2c_data, 1);
	*val = (i2c_data & 0x03) << 8;

	err = m5602_read_sensor(sd, OV9650_GAIN, &i2c_data, 1);
	*val |= i2c_data;
	PDEBUG(D_V4L2, "Read gain %d", *val);
	return err;
}

int ov9650_set_gain(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	/* The 2 MSB */
	/* Read the OV9650_VREF register first to avoid
	   corrupting the VREF high and low bits */
	m5602_read_sensor(sd, OV9650_VREF, &i2c_data, 1);
	/* Mask away all uninteresting bits */
	i2c_data = ((val & 0x0300) >> 2) |
			(i2c_data & 0x3F);
	err = m5602_write_sensor(sd, OV9650_VREF, &i2c_data, 1);

	/* The 8 LSBs */
	i2c_data = val & 0xff;
	err = m5602_write_sensor(sd, OV9650_GAIN, &i2c_data, 1);
	return err;
}

int ov9650_get_red_balance(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_RED, &i2c_data, 1);
	*val = i2c_data;

	PDEBUG(D_V4L2, "Read red gain %d", *val);

	return err;
}

int ov9650_set_red_balance(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set red gain to %d",
			     val & 0xff);

	i2c_data = val & 0xff;
	err = m5602_write_sensor(sd, OV9650_RED, &i2c_data, 1);

	return err;
}

int ov9650_get_blue_balance(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_BLUE, &i2c_data, 1);
	*val = i2c_data;

	PDEBUG(D_V4L2, "Read blue gain %d", *val);

	return err;
}

int ov9650_set_blue_balance(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set blue gain to %d",
	       val & 0xff);

	i2c_data = val & 0xff;
	err = m5602_write_sensor(sd, OV9650_BLUE, &i2c_data, 1);

	return err;
}

int ov9650_get_hflip(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_MVFP, &i2c_data, 1);
	if (dmi_check_system(ov9650_flip_dmi_table))
		*val = ((i2c_data & OV9650_HFLIP) >> 5) ? 0 : 1;
	else
		*val = (i2c_data & OV9650_HFLIP) >> 5;
	PDEBUG(D_V4L2, "Read horizontal flip %d", *val);

	return err;
}

int ov9650_set_hflip(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set horizontal flip to %d", val);
	err = m5602_read_sensor(sd, OV9650_MVFP, &i2c_data, 1);
	if (err < 0)
		return err;

	if (dmi_check_system(ov9650_flip_dmi_table))
		i2c_data = ((i2c_data & 0xdf) |
			   (((val ? 0 : 1) & 0x01) << 5));
	else
		i2c_data = ((i2c_data & 0xdf) |
			   ((val & 0x01) << 5));

	err = m5602_write_sensor(sd, OV9650_MVFP, &i2c_data, 1);

	return err;
}

int ov9650_get_vflip(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_MVFP, &i2c_data, 1);
	if (dmi_check_system(ov9650_flip_dmi_table))
		*val = ((i2c_data & 0x10) >> 4) ? 0 : 1;
	else
		*val = (i2c_data & 0x10) >> 4;
	PDEBUG(D_V4L2, "Read vertical flip %d", *val);

	return err;
}

int ov9650_set_vflip(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set vertical flip to %d", val);
	err = m5602_read_sensor(sd, OV9650_MVFP, &i2c_data, 1);
	if (err < 0)
		return err;

	if (dmi_check_system(ov9650_flip_dmi_table))
		i2c_data = ((i2c_data & 0xef) |
				(((val ? 0 : 1) & 0x01) << 4));
	else
		i2c_data = ((i2c_data & 0xef) |
				((val & 0x01) << 4));

	err = m5602_write_sensor(sd, OV9650_MVFP, &i2c_data, 1);

	return err;
}

int ov9650_get_brightness(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_VREF, &i2c_data, 1);
	if (err < 0)
		return err;
	*val = (i2c_data & 0x03) << 8;

	err = m5602_read_sensor(sd, OV9650_GAIN, &i2c_data, 1);
	*val |= i2c_data;
	PDEBUG(D_V4L2, "Read gain %d", *val);

	return err;
}

int ov9650_set_brightness(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set gain to %d", val & 0x3ff);

	/* Read the OV9650_VREF register first to avoid
		corrupting the VREF high and low bits */
	err = m5602_read_sensor(sd, OV9650_VREF, &i2c_data, 1);
	if (err < 0)
		return err;

	/* Mask away all uninteresting bits */
	i2c_data = ((val & 0x0300) >> 2) | (i2c_data & 0x3F);
	err = m5602_write_sensor(sd, OV9650_VREF, &i2c_data, 1);
	if (err < 0)
		return err;

	/* The 8 LSBs */
	i2c_data = val & 0xff;
	err = m5602_write_sensor(sd, OV9650_GAIN, &i2c_data, 1);

	return err;
}

int ov9650_get_auto_white_balance(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_COM8, &i2c_data, 1);
	*val = (i2c_data & OV9650_AWB_EN) >> 1;
	PDEBUG(D_V4L2, "Read auto white balance %d", *val);

	return err;
}

int ov9650_set_auto_white_balance(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set auto white balance to %d", val);
	err = m5602_read_sensor(sd, OV9650_COM8, &i2c_data, 1);
	if (err < 0)
		return err;

	i2c_data = ((i2c_data & 0xfd) | ((val & 0x01) << 1));
	err = m5602_write_sensor(sd, OV9650_COM8, &i2c_data, 1);

	return err;
}

int ov9650_get_auto_gain(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = m5602_read_sensor(sd, OV9650_COM8, &i2c_data, 1);
	*val = (i2c_data & OV9650_AGC_EN) >> 2;
	PDEBUG(D_V4L2, "Read auto gain control %d", *val);

	return err;
}

int ov9650_set_auto_gain(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u8 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set auto gain control to %d", val);
	err = m5602_read_sensor(sd, OV9650_COM8, &i2c_data, 1);
	if (err < 0)
		return err;

	i2c_data = ((i2c_data & 0xfb) | ((val & 0x01) << 2));
	err = m5602_write_sensor(sd, OV9650_COM8, &i2c_data, 1);

	return err;
}

static void ov9650_dump_registers(struct sd *sd)
{
	int address;
	info("Dumping the ov9650 register state");
	for (address = 0; address < 0xa9; address++) {
		u8 value;
		m5602_read_sensor(sd, address, &value, 1);
		info("register 0x%x contains 0x%x",
		     address, value);
	}

	info("ov9650 register state dump complete");

	info("Probing for which registers that are read/write");
	for (address = 0; address < 0xff; address++) {
		u8 old_value, ctrl_value;
		u8 test_value[2] = {0xff, 0xff};

		m5602_read_sensor(sd, address, &old_value, 1);
		m5602_write_sensor(sd, address, test_value, 1);
		m5602_read_sensor(sd, address, &ctrl_value, 1);

		if (ctrl_value == test_value[0])
			info("register 0x%x is writeable", address);
		else
			info("register 0x%x is read only", address);

		/* Restore original value */
		m5602_write_sensor(sd, address, &old_value, 1);
	}
}
