/*
 * linux/drivers/ide/pci/aec62xx.c		Version 0.26	Sep 1, 2007
 *
 * Copyright (C) 1999-2002	Andre Hedrick <andre@linux-ide.org>
 * Copyright (C) 2007		MontaVista Software, Inc. <source@mvista.com>
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/hdreg.h>
#include <linux/ide.h>
#include <linux/init.h>

#include <asm/io.h>

struct chipset_bus_clock_list_entry {
	u8 xfer_speed;
	u8 chipset_settings;
	u8 ultra_settings;
};

static const struct chipset_bus_clock_list_entry aec6xxx_33_base [] = {
	{	XFER_UDMA_6,	0x31,	0x07	},
	{	XFER_UDMA_5,	0x31,	0x06	},
	{	XFER_UDMA_4,	0x31,	0x05	},
	{	XFER_UDMA_3,	0x31,	0x04	},
	{	XFER_UDMA_2,	0x31,	0x03	},
	{	XFER_UDMA_1,	0x31,	0x02	},
	{	XFER_UDMA_0,	0x31,	0x01	},

	{	XFER_MW_DMA_2,	0x31,	0x00	},
	{	XFER_MW_DMA_1,	0x31,	0x00	},
	{	XFER_MW_DMA_0,	0x0a,	0x00	},
	{	XFER_PIO_4,	0x31,	0x00	},
	{	XFER_PIO_3,	0x33,	0x00	},
	{	XFER_PIO_2,	0x08,	0x00	},
	{	XFER_PIO_1,	0x0a,	0x00	},
	{	XFER_PIO_0,	0x00,	0x00	},
	{	0,		0x00,	0x00	}
};

static const struct chipset_bus_clock_list_entry aec6xxx_34_base [] = {
	{	XFER_UDMA_6,	0x41,	0x06	},
	{	XFER_UDMA_5,	0x41,	0x05	},
	{	XFER_UDMA_4,	0x41,	0x04	},
	{	XFER_UDMA_3,	0x41,	0x03	},
	{	XFER_UDMA_2,	0x41,	0x02	},
	{	XFER_UDMA_1,	0x41,	0x01	},
	{	XFER_UDMA_0,	0x41,	0x01	},

	{	XFER_MW_DMA_2,	0x41,	0x00	},
	{	XFER_MW_DMA_1,	0x42,	0x00	},
	{	XFER_MW_DMA_0,	0x7a,	0x00	},
	{	XFER_PIO_4,	0x41,	0x00	},
	{	XFER_PIO_3,	0x43,	0x00	},
	{	XFER_PIO_2,	0x78,	0x00	},
	{	XFER_PIO_1,	0x7a,	0x00	},
	{	XFER_PIO_0,	0x70,	0x00	},
	{	0,		0x00,	0x00	}
};

#define BUSCLOCK(D)	\
	((struct chipset_bus_clock_list_entry *) pci_get_drvdata((D)))


/*
 * TO DO: active tuning and correction of cards without a bios.
 */
static u8 pci_bus_clock_list (u8 speed, struct chipset_bus_clock_list_entry * chipset_table)
{
	for ( ; chipset_table->xfer_speed ; chipset_table++)
		if (chipset_table->xfer_speed == speed) {
			return chipset_table->chipset_settings;
		}
	return chipset_table->chipset_settings;
}

static u8 pci_bus_clock_list_ultra (u8 speed, struct chipset_bus_clock_list_entry * chipset_table)
{
	for ( ; chipset_table->xfer_speed ; chipset_table++)
		if (chipset_table->xfer_speed == speed) {
			return chipset_table->ultra_settings;
		}
	return chipset_table->ultra_settings;
}

static void aec6210_set_mode(ide_drive_t *drive, const u8 speed)
{
	ide_hwif_t *hwif	= HWIF(drive);
	struct pci_dev *dev	= hwif->pci_dev;
	u16 d_conf		= 0;
	u8 ultra = 0, ultra_conf = 0;
	u8 tmp0 = 0, tmp1 = 0, tmp2 = 0;
	unsigned long flags;

	local_irq_save(flags);
	/* 0x40|(2*drive->dn): Active, 0x41|(2*drive->dn): Recovery */
	pci_read_config_word(dev, 0x40|(2*drive->dn), &d_conf);
	tmp0 = pci_bus_clock_list(speed, BUSCLOCK(dev));
	d_conf = ((tmp0 & 0xf0) << 4) | (tmp0 & 0xf);
	pci_write_config_word(dev, 0x40|(2*drive->dn), d_conf);

	tmp1 = 0x00;
	tmp2 = 0x00;
	pci_read_config_byte(dev, 0x54, &ultra);
	tmp1 = ((0x00 << (2*drive->dn)) | (ultra & ~(3 << (2*drive->dn))));
	ultra_conf = pci_bus_clock_list_ultra(speed, BUSCLOCK(dev));
	tmp2 = ((ultra_conf << (2*drive->dn)) | (tmp1 & ~(3 << (2*drive->dn))));
	pci_write_config_byte(dev, 0x54, tmp2);
	local_irq_restore(flags);
}

static void aec6260_set_mode(ide_drive_t *drive, const u8 speed)
{
	ide_hwif_t *hwif	= HWIF(drive);
	struct pci_dev *dev	= hwif->pci_dev;
	u8 unit		= (drive->select.b.unit & 0x01);
	u8 tmp1 = 0, tmp2 = 0;
	u8 ultra = 0, drive_conf = 0, ultra_conf = 0;
	unsigned long flags;

	local_irq_save(flags);
	/* high 4-bits: Active, low 4-bits: Recovery */
	pci_read_config_byte(dev, 0x40|drive->dn, &drive_conf);
	drive_conf = pci_bus_clock_list(speed, BUSCLOCK(dev));
	pci_write_config_byte(dev, 0x40|drive->dn, drive_conf);

	pci_read_config_byte(dev, (0x44|hwif->channel), &ultra);
	tmp1 = ((0x00 << (4*unit)) | (ultra & ~(7 << (4*unit))));
	ultra_conf = pci_bus_clock_list_ultra(speed, BUSCLOCK(dev));
	tmp2 = ((ultra_conf << (4*unit)) | (tmp1 & ~(7 << (4*unit))));
	pci_write_config_byte(dev, (0x44|hwif->channel), tmp2);
	local_irq_restore(flags);
}

static void aec_set_pio_mode(ide_drive_t *drive, const u8 pio)
{
	drive->hwif->set_dma_mode(drive, pio + XFER_PIO_0);
}

static void aec62xx_dma_lost_irq (ide_drive_t *drive)
{
	switch (HWIF(drive)->pci_dev->device) {
		case PCI_DEVICE_ID_ARTOP_ATP860:
		case PCI_DEVICE_ID_ARTOP_ATP860R:
		case PCI_DEVICE_ID_ARTOP_ATP865:
		case PCI_DEVICE_ID_ARTOP_ATP865R:
			printk(" AEC62XX time out ");
		default:
			break;
	}
}

static unsigned int __devinit init_chipset_aec62xx(struct pci_dev *dev, const char *name)
{
	int bus_speed = system_bus_clock();

	if (bus_speed <= 33)
		pci_set_drvdata(dev, (void *) aec6xxx_33_base);
	else
		pci_set_drvdata(dev, (void *) aec6xxx_34_base);

	/* These are necessary to get AEC6280 Macintosh cards to work */
	if ((dev->device == PCI_DEVICE_ID_ARTOP_ATP865) ||
	    (dev->device == PCI_DEVICE_ID_ARTOP_ATP865R)) {
		u8 reg49h = 0, reg4ah = 0;
		/* Clear reset and test bits.  */
		pci_read_config_byte(dev, 0x49, &reg49h);
		pci_write_config_byte(dev, 0x49, reg49h & ~0x30);
		/* Enable chip interrupt output.  */
		pci_read_config_byte(dev, 0x4a, &reg4ah);
		pci_write_config_byte(dev, 0x4a, reg4ah & ~0x01);
		/* Enable burst mode. */
		pci_read_config_byte(dev, 0x4a, &reg4ah);
		pci_write_config_byte(dev, 0x4a, reg4ah | 0x80);
	}

	return dev->irq;
}

static void __devinit init_hwif_aec62xx(ide_hwif_t *hwif)
{
	struct pci_dev *dev	= hwif->pci_dev;

	hwif->set_pio_mode = &aec_set_pio_mode;

	if (dev->device == PCI_DEVICE_ID_ARTOP_ATP850UF)
		hwif->set_dma_mode = &aec6210_set_mode;
	else
		hwif->set_dma_mode = &aec6260_set_mode;

	if (hwif->dma_base == 0)
		return;

	hwif->dma_lost_irq	= &aec62xx_dma_lost_irq;

	if (dev->device == PCI_DEVICE_ID_ARTOP_ATP850UF)
		return;

	if (hwif->cbl != ATA_CBL_PATA40_SHORT) {
		u8 ata66 = 0, mask = hwif->channel ? 0x02 : 0x01;

		pci_read_config_byte(hwif->pci_dev, 0x49, &ata66);

		hwif->cbl = (ata66 & mask) ? ATA_CBL_PATA40 : ATA_CBL_PATA80;
	}
}

static ide_pci_device_t aec62xx_chipsets[] __devinitdata = {
	{	/* 0 */
		.name		= "AEC6210",
		.init_chipset	= init_chipset_aec62xx,
		.init_hwif	= init_hwif_aec62xx,
		.enablebits	= {{0x4a,0x02,0x02}, {0x4a,0x04,0x04}},
		.host_flags	= IDE_HFLAG_SERIALIZE |
				  IDE_HFLAG_NO_ATAPI_DMA |
				  IDE_HFLAG_OFF_BOARD,
		.pio_mask	= ATA_PIO4,
		.mwdma_mask	= ATA_MWDMA2,
		.udma_mask	= ATA_UDMA2,
	},{	/* 1 */
		.name		= "AEC6260",
		.init_chipset	= init_chipset_aec62xx,
		.init_hwif	= init_hwif_aec62xx,
		.host_flags	= IDE_HFLAG_NO_ATAPI_DMA | IDE_HFLAG_NO_AUTODMA |
				  IDE_HFLAG_OFF_BOARD,
		.pio_mask	= ATA_PIO4,
		.mwdma_mask	= ATA_MWDMA2,
		.udma_mask	= ATA_UDMA4,
	},{	/* 2 */
		.name		= "AEC6260R",
		.init_chipset	= init_chipset_aec62xx,
		.init_hwif	= init_hwif_aec62xx,
		.enablebits	= {{0x4a,0x02,0x02}, {0x4a,0x04,0x04}},
		.host_flags	= IDE_HFLAG_NO_ATAPI_DMA,
		.pio_mask	= ATA_PIO4,
		.mwdma_mask	= ATA_MWDMA2,
		.udma_mask	= ATA_UDMA4,
	},{	/* 3 */
		.name		= "AEC6280",
		.init_chipset	= init_chipset_aec62xx,
		.init_hwif	= init_hwif_aec62xx,
		.host_flags	= IDE_HFLAG_NO_ATAPI_DMA | IDE_HFLAG_OFF_BOARD,
		.pio_mask	= ATA_PIO4,
		.mwdma_mask	= ATA_MWDMA2,
		.udma_mask	= ATA_UDMA5,
	},{	/* 4 */
		.name		= "AEC6280R",
		.init_chipset	= init_chipset_aec62xx,
		.init_hwif	= init_hwif_aec62xx,
		.enablebits	= {{0x4a,0x02,0x02}, {0x4a,0x04,0x04}},
		.host_flags	= IDE_HFLAG_NO_ATAPI_DMA | IDE_HFLAG_OFF_BOARD,
		.pio_mask	= ATA_PIO4,
		.mwdma_mask	= ATA_MWDMA2,
		.udma_mask	= ATA_UDMA5,
	}
};

/**
 *	aec62xx_init_one	-	called when a AEC is found
 *	@dev: the aec62xx device
 *	@id: the matching pci id
 *
 *	Called when the PCI registration layer (or the IDE initialization)
 *	finds a device matching our IDE device tables.
 *
 *	NOTE: since we're going to modify the 'name' field for AEC-6[26]80[R]
 *	chips, pass a local copy of 'struct pci_device_id' down the call chain.
 */
 
static int __devinit aec62xx_init_one(struct pci_dev *dev, const struct pci_device_id *id)
{
	ide_pci_device_t d;
	u8 idx = id->driver_data;

	d = aec62xx_chipsets[idx];

	if (idx == 3 || idx == 4) {
		unsigned long dma_base = pci_resource_start(dev, 4);

		if (inb(dma_base + 2) & 0x10) {
			d.name = (idx == 4) ? "AEC6880R" : "AEC6880";
			d.udma_mask = ATA_UDMA6;
		}
	}

	return ide_setup_pci_device(dev, &d);
}

static const struct pci_device_id aec62xx_pci_tbl[] = {
	{ PCI_VDEVICE(ARTOP, PCI_DEVICE_ID_ARTOP_ATP850UF), 0 },
	{ PCI_VDEVICE(ARTOP, PCI_DEVICE_ID_ARTOP_ATP860),   1 },
	{ PCI_VDEVICE(ARTOP, PCI_DEVICE_ID_ARTOP_ATP860R),  2 },
	{ PCI_VDEVICE(ARTOP, PCI_DEVICE_ID_ARTOP_ATP865),   3 },
	{ PCI_VDEVICE(ARTOP, PCI_DEVICE_ID_ARTOP_ATP865R),  4 },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, aec62xx_pci_tbl);

static struct pci_driver driver = {
	.name		= "AEC62xx_IDE",
	.id_table	= aec62xx_pci_tbl,
	.probe		= aec62xx_init_one,
};

static int __init aec62xx_ide_init(void)
{
	return ide_pci_register_driver(&driver);
}

module_init(aec62xx_ide_init);

MODULE_AUTHOR("Andre Hedrick");
MODULE_DESCRIPTION("PCI driver module for ARTOP AEC62xx IDE");
MODULE_LICENSE("GPL");
