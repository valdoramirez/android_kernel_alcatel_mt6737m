/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>

#include "fp_drv.h"

#include "../drivers/spi/mediatek/mt6735/mt_spi.h"

#define MAX_DRV_NUM (3)
#define GF_DEV_NAME "goodix_fp"
#define SL_DEV_NAME  "silead_fp_dev"

#if 1
 struct mt_chip_conf spi_data = {
		.setuptime =10,//6, //10,//15,//10 ,//3,
		.holdtime =10, //6,//10,//15,//10,//3,
		.high_time =30,//4,//6,//8,//12, //25,//8,      //10--6m   15--4m   20--3m  30--2m  [ 60--1m 120--0.5m  300--0.2m]
		.low_time = 30,//4,//6,//8,//12,//25,//8,
		.cs_idletime = 2,//30,// 60,//100,//12,
		.ulthgh_thrsh = 0,

		.rx_mlsb = SPI_MSB, 
		.tx_mlsb = SPI_MSB,		 
		.tx_endian = 0,
		.rx_endian = 0,

		.cpol = SPI_CPOL_0,
		.cpha = SPI_CPHA_0,
		
//		.com_mod = FIFO_TRANSFER,
		.com_mod = DMA_TRANSFER,
		.pause = 0,
		.finish_intr = 1,
};
#endif
///////////////////////////////////////////////////////////////////
static int fp_probe(struct platform_device *pdev);
static int fp_remove(struct platform_device *pdev);
///////////////////////////////////////////////////////////////////

static struct platform_driver fp_driver = {
	.probe = fp_probe,
	.remove = fp_remove,
	.driver = {
		   .name = "fp_drv",
	},
};


struct platform_device fp_device = {
    .name   	= "fp_drv",
    .id        	= -1,
};

static struct fp_driver_t *g_fp_drv = NULL;
static struct fp_driver_t fp_driver_list[MAX_DRV_NUM];

int fp_driver_load(struct fp_driver_t *drv)
{
	int i;

	__FUN();

	if((drv == NULL) ||(g_fp_drv != NULL))
	{
		return -1;
	}

	for(i = 0; i < MAX_DRV_NUM; i++)
	{
		if(fp_driver_list[i].device_name == NULL)
		{
			fp_driver_list[i].device_name = drv->device_name;
			fp_driver_list[i].local_init = drv->local_init;
			fp_driver_list[i].local_uninit = drv->local_uninit;
			fp_driver_list[i].init_ok = drv->init_ok;
			printk("---Add [%s] to list-----\n", fp_driver_list[i].device_name);
		}

		if (strcmp(fp_driver_list[i].device_name, drv->device_name) == 0) {
			break;
		}
	}

	return 0;
}

int fp_driver_remove(struct fp_driver_t *drv)
{
	int i = 0;
	
	__FUN();
	
	if (drv == NULL) {
		return -1;
	}
	
	for (i = 0; i < MAX_DRV_NUM; i++) 
	{
		if(strcmp(fp_driver_list[i].device_name, drv->device_name) == 0) {
			memset(&fp_driver_list[i], 0, sizeof(struct fp_driver_t));
			printk("remove drv:%s\n", drv->device_name);
			break;
		}
	}
	
	return 0;
}

static ssize_t info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char fp_dev_info[32] = "NULL:NULL:NULL:NULL";
	if (g_fp_drv == NULL)
	{
		sprintf(fp_dev_info, "%s:%s:%s:%s", "N0", "NULL", "NULL", "NULL"); 
		return sprintf(buf, "%s\n", fp_dev_info);
	}

	if (strcmp(g_fp_drv->device_name, GF_DEV_NAME) == 0) {
		sprintf(fp_dev_info, "%s:%s:%s:%s", GF_DEV_NAME, "GF", "SUP1", "NULL");
	}
	else if (strcmp(g_fp_drv->device_name, SL_DEV_NAME) == 0)
	{
		sprintf(fp_dev_info, "%s:%s:%s:%s", SL_DEV_NAME, "6172", "SUP2", "NULL");
	} 
	return sprintf(buf, "%s\n", fp_dev_info);  
}

DEVICE_ATTR(FP, S_IRUGO, info_show, NULL);
extern struct device *get_deviceinfo_dev(void);
static int create_fp_node(void)
{
	struct device *fp_dev;
	fp_dev = get_deviceinfo_dev();
	if (device_create_file(fp_dev, &dev_attr_FP) < 0)
	{
		printk("Failed to create fp node.\n");
	}
	return 0;
}



static struct spi_board_info spi_board_devs[] __initdata = {
	[0] = {
    .modalias="fingerprint",
		.bus_num = 0,
		.chip_select=0,
		.mode = SPI_MODE_0,
		.controller_data=&spi_data,
	},
};

static int fp_probe(struct platform_device *pdev)
{
	int i;
	int found = 0;
	__FUN();

	spi_register_board_info(spi_board_devs,ARRAY_SIZE(spi_board_devs));

	for(i = 0; i < MAX_DRV_NUM; i++)
	{
		if(fp_driver_list[i].device_name)
		{
			printk("------Start drv init :[%s]------\n", fp_driver_list[i].device_name);
			fp_driver_list[i].local_init();
			msleep(200);
			if(fp_driver_list[i].init_ok())
			{
				found = 1;
				g_fp_drv = &fp_driver_list[i];
				break;
			}
		
			fp_driver_list[i].local_uninit();
		}
	}

	if(!found)
	{
		printk("#########Not match any fp-spi device, please check driver !!!#######\n");
	}

	create_fp_node();

	return 0;
}

static int fp_remove(struct platform_device *pdev)
{
	__FUN();
	device_remove_file(&pdev->dev, &dev_attr_FP);
	return 0;
}

static int __init fp_drv_init(void)
{
	__FUN();

	if (platform_device_register(&fp_device) != 0) {
		printk( "device_register fail!.\n");
		return -1;
	
	}
	
	if (platform_driver_register(&fp_driver) != 0) {
		printk( "driver_register fail!.\n");
		return -1;
	}
	
	return 0;
}

static void __exit fp_drv_exit(void)
{
	__FUN();
	platform_driver_unregister(&fp_driver);
}

late_initcall(fp_drv_init);
module_exit(fp_drv_exit);

