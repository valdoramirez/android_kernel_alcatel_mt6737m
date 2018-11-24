



#include "AudDrv_NXP.h"
#if 1//defined(CONFIG_MTK_LEGACY)
//#include <mach/mt_gpio.h>
#include "mt_gpio.h"
#include <mach/gpio_const.h>
#include "mt-plat/mtgpio.h"
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#include <mach/gpio_const.h>
#endif
//#include "cust_gpio_usage.h"

#define TFA_I2C_CHANNEL     (1)

#ifdef CONFIG_MTK_NXP_TFA9890
//#error(tfa9890);
#define ECODEC_SLAVE_ADDR_WRITE 0x68
#define ECODEC_SLAVE_ADDR_READ  0x69
#else
#error(tfa9887);
#define ECODEC_SLAVE_ADDR_WRITE 0x6c
#define ECODEC_SLAVE_ADDR_READ  0x6d
#endif

#define I2C_MASTER_CLOCK       400
#define NXPEXTSPK_I2C_DEVNAME "mtksmartpa"


#define AUDDRV_NXPSPK_NAME   "mtksmartpa"
#define AUDDRV_AUTHOR "MediaTek WCX"
#define RW_BUFFER_LENGTH (256)

#define smart_set_gpio(x) (x|0x80000000)
#define GPIO_SMARTPA_RST_PIN smart_set_gpio(60)


/* I2C variable */
static struct i2c_client *new_client;
static char WriteBuffer[RW_BUFFER_LENGTH];
static char ReadBuffer[RW_BUFFER_LENGTH];

static u8 *TfaI2CDMABuf_va;
static unsigned long TfaI2CDMABuf_pa;

#ifdef CONFIG_OF
#if 1
static struct pinctrl *TFA98xx_pinctrl;
enum{
    GPIO_TFA98xx_DEF = 0,
	GPIO_TFA98xx_RST_LOW,
	GPIO_TFA98xx_RST_HIGH,
	GPIO_TFA98xx_END
};

struct audio_gpio_attr {
	const char *name;
	bool gpio_prepare;
	struct pinctrl_state *gpioctrl;
};

static struct audio_gpio_attr tfa98xx_gpios[GPIO_TFA98xx_END] = {
    [GPIO_TFA98xx_DEF] = {"default", false, NULL},
	[GPIO_TFA98xx_RST_LOW] = {"rst_low", false, NULL},
	[GPIO_TFA98xx_RST_HIGH] = {"rst_high", false, NULL},
};

static int smartpa_parse_gpio(void *dev)
{
    int i;
	int ret;
	
	printk("+%s\n", __func__);
	TFA98xx_pinctrl = devm_pinctrl_get(dev);
	
	printk("%s(%d) lookup pin state\n", __func__, __LINE__);
	for (i = 0; i < ARRAY_SIZE(tfa98xx_gpios); i++) {
		tfa98xx_gpios[i].gpioctrl = pinctrl_lookup_state(TFA98xx_pinctrl, tfa98xx_gpios[i].name);
		if (IS_ERR(tfa98xx_gpios[i].gpioctrl)) {
			ret = PTR_ERR(tfa98xx_gpios[i].gpioctrl);
			printk("%s pinctrl_lookup_state %s fail %d\n", __func__, tfa98xx_gpios[i].name, ret);
		} else {
			printk("%s(%d)\n",__FUNCTION__,__LINE__);
			tfa98xx_gpios[i].gpio_prepare = true;
		}
	}
	printk("-%s\n", __func__);
	return 0;
}
#endif

#endif


/* new I2C register method */
static const struct i2c_device_id nxpExt_i2c_id[] = { {NXPEXTSPK_I2C_DEVNAME, 0}, {} };
//static struct i2c_board_info nxpExt_dev __initdata = {
//	I2C_BOARD_INFO(NXPEXTSPK_I2C_DEVNAME, 0x34) };

/* function declration */
static int NXPExtSpk_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int NXPExtSpk_i2c_remove(struct i2c_client *client);
/* void AudDrv_NXPSpk_Init(void); */
/* bool NXPExtSpk_Register(void); */
/* static int NXPExtSpk_register(void); */
/* ssize_t NXPSpk_read_byte(u8 addr, u8 *returnData); */

static int NXPExtSpk_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, NXPEXTSPK_I2C_DEVNAME);
	return 0;
}

static int NXPExtSpk_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	return 0;
}

static int NXPExtSpk_i2c_resume(struct i2c_client *client)
{
	return 0;
}

/* i2c driver */
static struct of_device_id tfa98xx_match_tbl[] = {
	{ .compatible = "mediatek,SMARTPA", },
	{ },
};

static struct i2c_driver NXPExtSpk_i2c_driver = {
	.probe = NXPExtSpk_i2c_probe,
	.remove = NXPExtSpk_i2c_remove,
	.detect = NXPExtSpk_i2c_detect,
	.suspend = NXPExtSpk_i2c_suspend,
	.resume = NXPExtSpk_i2c_resume,
	.id_table = nxpExt_i2c_id,
	.driver = {
		   .name = NXPEXTSPK_I2C_DEVNAME,
		   .of_match_table = tfa98xx_match_tbl,
	},
};


static int NXPExtSpk_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	new_client = client;
	new_client->timing = 400;
	printk("NXPExtSpk_i2c_probe\n");
#ifdef CONFIG_MTK_NXP_TFA9890
	pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_LOW].gpioctrl);
	usleep_range(2*1000, 3*1000);
	pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_HIGH].gpioctrl);
	usleep_range(2*1000, 3*1000);
	pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_LOW].gpioctrl);
	usleep_range(10*1000, 20*1000);
#endif

	TfaI2CDMABuf_va = (u8 *) dma_alloc_coherent(NULL, 4096, (dma_addr_t *)&TfaI2CDMABuf_pa, GFP_KERNEL);
	//TfaI2CDMABuf_va = (u8 *) dma_alloc_coherent(&(client->dev), 4096, (dma_addr_t *)&TfaI2CDMABuf_pa, GFP_KERNEL);
	if (!TfaI2CDMABuf_va) {
		NXP_ERROR("dma_alloc_coherent error\n");
		NXP_INFO("i2c_probe failed\n");
		return -1;
	}
	NXP_INFO("i2c_probe success\n");
	return 0;
}

static int NXPExtSpk_i2c_remove(struct i2c_client *client)
{
	new_client = NULL;
	i2c_unregister_device(client);
	i2c_del_driver(&NXPExtSpk_i2c_driver);
	if (TfaI2CDMABuf_va) {
		dma_free_coherent(NULL, 4096, TfaI2CDMABuf_va, TfaI2CDMABuf_pa);
		TfaI2CDMABuf_va = NULL;
		TfaI2CDMABuf_pa = 0;
	}
	usleep_range(1*1000, 2*1000);
#ifdef CONFIG_MTK_NXP_TFA9890
    pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_LOW].gpioctrl);
#endif
	return 0;
}

/* read write implementation */
/* read one register */
ssize_t NXPSpk_read_byte(u8 addr, u8 *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;
	cmd_buf[0] = addr;

	if (!new_client) {
		pr_warn("NXPSpk_read_byte I2C client not initialized!!");
		return -1;
	}
	ret = i2c_master_send(new_client, &cmd_buf[0], 1);
	if (ret < 0) {
		pr_warn("NXPSpk_read_byte read sends command error!!\n");
		return -1;
	}
	ret = i2c_master_recv(new_client, &readData, 1);
	if (ret < 0) {
		pr_warn("NXPSpk_read_byte reads recv data error!!\n");
		return -1;
	}
	*returnData = readData;
	/* pr_debug("addr 0x%x data 0x%x\n", addr, readData); */
	return 0;
}

/* write register */
ssize_t NXPExt_write_byte(u8 addr, u8 writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;
	if (!new_client) {
		pr_warn("I2C client not initialized!!");
		return -1;
	}
	write_data[0] = addr;	/* ex. 0x01 */
	write_data[1] = writeData;
	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		pr_warn("write sends command error!!");
		return -1;
	}
	/* pr_debug("addr 0x%x data 0x%x\n", addr, writeData); */
	return 0;
}


static int NXPExtSpk_register(void)
{
	pr_debug("NXPExtSpk_register\n");

#ifdef CONFIG_MTK_NXP_TFA9890
	pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_HIGH].gpioctrl);
	usleep_range(1*1000, 2*1000);
#endif

//	i2c_register_board_info(TFA_I2C_CHANNEL, &nxpExt_dev, 1);
	if (i2c_add_driver(&NXPExtSpk_i2c_driver)) {
		pr_warn("fail to add device into i2c");
		return -1;
	}
	return 0;
}


bool NXPExtSpk_Register(void)
{
	pr_debug("NXPExtSpk_Register\n");
	NXPExtSpk_register();
	return true;
}

void AudDrv_NXPSpk_Init(void)
{
#if 0
	printk("Set GPIO for AFE I2S output to external DAC\n");
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_lrck), pin_nxpspk_lrck_mode);
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_bck), pin_nxpspk_bck_mode);
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_datai), pin_nxpspk_datai_mode);
	mt_set_gpio_mode(smart_set_gpio(pin_nxpspk_datao), pin_nxpspk_datao_mode);
#endif
}

#define NXPSMARTPAMAGIC		'N'
#define NXP_SMARTPA_RST_SET		_IO(NXPSMARTPAMAGIC, 1)
#define NXP_SMARTPA_RST_RSET	_IO(NXPSMARTPAMAGIC, 2)
static long AudDrv_nxpspk_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	/* pr_debug("AudDrv_nxpspk_ioctl cmd = 0x%x arg = %lu\n", cmd, arg); */

	switch (cmd) {
	case NXP_SMARTPA_RST_SET:
		printk("%s(%d) NXP_SMARTPA_RST_SET\n", __FUNCTION__, __LINE__);
		pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_HIGH].gpioctrl);
		usleep_range(1*1000, 2*1000);
		break;

	case NXP_SMARTPA_RST_RSET:
		printk("%s(%d) NXP_SMARTPA_RST_RESET\n", __FUNCTION__, __LINE__);
		pinctrl_select_state(TFA98xx_pinctrl, tfa98xx_gpios[GPIO_TFA98xx_RST_LOW].gpioctrl);
		usleep_range(1*1000, 2*1000);
		break;

	default:{
			/* pr_debug("AudDrv_nxpspk_ioctl Fail command: %x\n", cmd); */
			ret = 0;
			break;
		}
	}
	return ret;
}

static int AudDrv_nxpspk_probe(struct platform_device *pdev)
{
	int ret = 0;
	printk("AudDrv_nxpspk_probe\n");

	if (ret < 0)
		pr_warn("AudDrv_nxpspk_probe request_irq MT6582_AP_BT_CVSD_IRQ_LINE Fail\n");
	smartpa_parse_gpio(&pdev->dev);
	NXPExtSpk_Register();
	AudDrv_NXPSpk_Init();

	memset((void *)WriteBuffer, 0, RW_BUFFER_LENGTH);
	memset((void *)ReadBuffer, 0, RW_BUFFER_LENGTH);

	printk("-AudDrv_nxpspk_probe\n");
	return 0;
}

static int AudDrv_nxpspk_open(struct inode *inode, struct file *fp)
{
	return 0;
}

static int nxp_i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.timing = I2C_MASTER_CLOCK;
	msg.addr = (client->addr & I2C_MASK_FLAG);

	if (count <= 8)
		msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG );
	else
		msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);

	msg.flags = 0;
	/* msg.timing = client->timing; */

	msg.len = count;
	msg.buf = (char *)buf;
	//msg.ext_flag = client->ext_flag;
	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}

static int nxp_i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.timing = I2C_MASTER_CLOCK;
	//msg.flags = client->flags & I2C_M_TEN;
	msg.addr = client->addr & I2C_MASK_FLAG;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.ext_flag = client->ext_flag;
	msg.buf = (char *)buf;

	if (count <= 8)
		msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG );
	else
		msg.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg received), return #bytes received,
	 * else error code.
	 */
	return (ret == 1) ? count : ret;
}

static ssize_t AudDrv_nxpspk_write(struct file *fp, const char __user *data, size_t count,
				   loff_t *offset)
{
	int i = 0;
	int ret;
	char *tmp;
	char *TfaI2CDMABuf = (char *)TfaI2CDMABuf_va;
printk("[lee]:%s-------%d\n",__func__,__LINE__);
	/* count = 8192; */

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	if (copy_from_user(tmp, data, count)) {
		kfree(tmp);
		return -EFAULT;
	}
	/* NXP_INFO("i2c-dev: i2c-%d writing %zu bytes.\n", iminor(file->f_path.dentry->d_inode), count); */

	for (i = 0; i < count; i++)
		TfaI2CDMABuf[i] = tmp[i];

	if (count <= 8) {
		/* /new_client->addr = new_client->addr & I2C_MASK_FLAG;  //cruson */
		ret = nxp_i2c_master_send(new_client, tmp, count);
	} else {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG |I2C_ENEXT_FLAG;  //cruson */
		ret = nxp_i2c_master_send(new_client, (char *)TfaI2CDMABuf_pa, count);
	}
	kfree(tmp);
	return ret;
}

static ssize_t AudDrv_nxpspk_read(struct file *fp, char __user *data, size_t count,
				  loff_t *offset)
{
	int i = 0;
	char *tmp;
	char *TfaI2CDMABuf = (char *)TfaI2CDMABuf_va;
	int ret = 0;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;

	/* NXP_INFO("i2c-dev: i2c-%d reading %zu bytes.\n", iminor(file->f_path.dentry->d_inode), count); */

	if (count <= 8) {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG;  //cruson */
		ret = nxp_i2c_master_recv(new_client, tmp, count);
	} else {
		/* new_client->addr = new_client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG |I2C_ENEXT_FLAG;  //cruson */
		ret = nxp_i2c_master_recv(new_client, (char *)TfaI2CDMABuf_pa, count);
		for (i = 0; i < count; i++)
			tmp[i] = TfaI2CDMABuf[i];
	}

	if (ret >= 0)
		ret = copy_to_user(data, tmp, count) ? (-EFAULT) : ret;
	kfree(tmp);
	return ret;
}


static const struct file_operations AudDrv_nxpspk_fops = {
	.owner = THIS_MODULE,
	.open = AudDrv_nxpspk_open,
	.unlocked_ioctl = AudDrv_nxpspk_ioctl,
	.write = AudDrv_nxpspk_write,
	.read = AudDrv_nxpspk_read,
};

#ifdef CONFIG_MTK_NXP_TFA9890
static struct miscdevice AudDrv_nxpspk_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "smartpa_i2c",
	.fops = &AudDrv_nxpspk_fops,
};
#else
static struct miscdevice AudDrv_nxpspk_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "nxpspk",
	.fops = &AudDrv_nxpspk_fops,
};
#endif


#ifdef CONFIG_OF
static const struct of_device_id mtk_smart_pa_of_ids[] = {
	{.compatible = "NXP,tfa9890",},
	{}
};
#endif

static struct platform_driver AudDrv_nxpspk = {
	.probe = AudDrv_nxpspk_probe,
	.driver = {
		   .name = "AudioMTKNXPSPK",//NXPEXTSPK_I2C_DEVNAME,
//		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mtk_smart_pa_of_ids,
#endif
		   },
};

//static struct platform_device *AudDrv_NXPSpk_dev;

static int AudDrv_nxpspk_mod_init(void)
{
	int ret = 0;
	printk("+AudDrv_nxpspk_mod_init\n");

#ifndef CONFIG_OF
	printk("platform_device_alloc\n");
	AudDrv_NXPSpk_dev = platform_device_alloc("AudioMTKNXPSPK", -1);
	if (!AudDrv_NXPSpk_dev)
		return -ENOMEM;

	printk("platform_device_add\n");

	ret = platform_device_add(AudDrv_NXPSpk_dev);
	if (ret != 0) {
		platform_device_put(AudDrv_NXPSpk_dev);
		return ret;
	}
#endif
	/* Register platform DRIVER */

	ret = platform_driver_register(&AudDrv_nxpspk);
	if (ret) {
		pr_warn("AudDrv Fail:%d - Register DRIVER\n", ret);
		return ret;
	}
	/* register MISC device */
	ret = misc_register(&AudDrv_nxpspk_device);
	if (ret) {
		pr_warn("AudDrv_nxpspk_mod_init misc_register Fail:%d\n", ret);
		return ret;
	}

	printk("-AudDrv_nxpspk_mod_init\n");
	return 0;
}

static void AudDrv_nxpspk_mod_exit(void)
{
	printk("+AudDrv_nxpspk_mod_exit\n");

	printk("-AudDrv_nxpspk_mod_exit\n");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(AUDDRV_NXPSPK_NAME);
MODULE_AUTHOR(AUDDRV_AUTHOR);

module_init(AudDrv_nxpspk_mod_init);
module_exit(AudDrv_nxpspk_mod_exit);
