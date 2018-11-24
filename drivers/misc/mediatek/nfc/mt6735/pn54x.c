#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/wakelock.h>
#include <linux/dma-mapping.h>
#include <mt_clkbuf_ctl.h>  /*add for clock control*/
#include "pn54x.h"

#ifdef CONFIG_TCT_ROBUST
int nfc_init_flag = 0;
#endif

static struct platform_device *g_pn54x_platf_dev;
static struct PN54X_DEV    *g_pn54x_dev;
static struct of_device_id pn54x_platform_match_table = {.compatible = "mediatek,nfc",};
static struct of_device_id pn54x_i2c_match_table      = {.compatible = "mediatek,nfc",};
static struct i2c_device_id pn54x_i2c_id_table        = {PN54X_DEV_NAME, 0};

static struct platform_driver pn54x_platform_driver =
{
	.probe  = pn54x_platform_probe,
	.remove = pn54x_platform_remove,
	.driver =
	{
		.owner          = THIS_MODULE,
		.name           = PN54X_DEV_NAME,
		.of_match_table = &pn54x_platform_match_table,
	},
};

static struct i2c_driver pn54x_i2c_driver =
{
	.id_table   = &pn54x_i2c_id_table,
	.probe		= pn54x_i2c_probe,
	.remove		= pn54x_i2c_remove,
	.detect     = pn54x_i2c_detect,
	.driver		=
	{
		.owner	        = THIS_MODULE,
		.name	        = PN54X_DEV_NAME,
		.of_match_table = &pn54x_i2c_match_table,
	},
};

static const struct file_operations pn54x_dev_fops =
{
	.owner	        = THIS_MODULE,
	.llseek	        = no_llseek,
	.read	        = pn54x_dev_read,
	.write	        = pn54x_dev_write,
	.open	        = pn54x_dev_open,
	.unlocked_ioctl = pn54x_dev_ioctl,
};

DEVICE_ATTR(nfc, S_IRUGO, pn54x_show_info, NULL);

static int pn54x_pinctrl_init(struct platform_device *pdev, struct PIN_CTRL **dev_gpio)
{
	int    ret            = 0;
	struct PIN_CTRL *gpio = NULL;

	if (pdev == NULL)
	{
		pr_err("%s pdev NULL\n", __func__);
		return -EINVAL;
	}

	gpio = kzalloc(sizeof(struct PIN_CTRL), GFP_KERNEL);
	if (gpio == NULL)
	{
		pr_err("%s allocate memory for pin ctrl failed\n", __func__);
		return -ENOMEM;
	}

	gpio->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(gpio->pinctrl))
	{
		ret = PTR_ERR(gpio->pinctrl);
		pr_err("%s can not find nfc pinctrl\n", __func__);
		goto err_pinctrl_get;
	}

	gpio->ven_high = pinctrl_lookup_state(gpio->pinctrl, "ven_high");
	if (IS_ERR(gpio->ven_high))
	{
		ret = PTR_ERR(gpio->ven_high);
		pr_err("%s can not find nfc pinctrl ven h\n", __func__);
		goto err_pinctrl_lookup;
	}

	gpio->ven_low = pinctrl_lookup_state(gpio->pinctrl, "ven_low");
	if (IS_ERR(gpio->ven_low))
	{
		ret = PTR_ERR(gpio->ven_low);
		pr_err("%s can not find nfc pinctrl ven l\n", __func__);
		goto err_pinctrl_lookup;
	}

	gpio->dl_high = pinctrl_lookup_state(gpio->pinctrl, "dl_high");
	if (IS_ERR(gpio->dl_high))
	{
		ret = PTR_ERR(gpio->dl_high);
		pr_err("%s can not find nfc pinctrl dl h\n", __func__);
		goto err_pinctrl_lookup;
	}

	gpio->dl_low = pinctrl_lookup_state(gpio->pinctrl, "dl_low");
	if (IS_ERR(gpio->dl_low))
	{
		ret = PTR_ERR(gpio->dl_low);
		pr_err("%s can not find nfc pinctrl dl l\n", __func__);
		goto err_pinctrl_lookup;
	}

	gpio->irq_cfg = pinctrl_lookup_state(gpio->pinctrl, "irq_cfg");
	if (IS_ERR(gpio->irq_cfg))
	{
		ret = PTR_ERR(gpio->irq_cfg);
		pr_err("%s can not find nfc pinctrl irq cfg\n", __func__);
		goto err_pinctrl_lookup;
	}

	gpio->clk_irq_cfg = pinctrl_lookup_state(gpio->pinctrl, "clk_irq_cfg");
	if (IS_ERR(gpio->clk_irq_cfg))
	{
		ret = PTR_ERR(gpio->clk_irq_cfg);
		pr_err("%s can not find nfc pinctrl clk irq cfg\n", __func__);
		goto err_pinctrl_lookup;
	}

	*dev_gpio = gpio;
	return 0;

err_pinctrl_lookup:
	devm_pinctrl_put(gpio->pinctrl);
err_pinctrl_get:
	kfree(gpio);
	gpio = NULL;
	return ret;
}

static int pn54x_check_device(const struct i2c_client *client, const struct PIN_CTRL *gpio)
{
	int ret;
	char send_reset[] = {0x20, 0x00, 0x01, 0x01}; /* PN54x RSET Frame */

	if (client == NULL || gpio == NULL)
	{
		pr_err("%s client or gpio NULL\n", __func__);
		return -EINVAL;
	}

	pinctrl_select_state(gpio->pinctrl, gpio->dl_low);
	pinctrl_select_state(gpio->pinctrl, gpio->ven_high);
	msleep(10);
	pinctrl_select_state(gpio->pinctrl, gpio->ven_low);
	msleep(10);
	pinctrl_select_state(gpio->pinctrl, gpio->ven_high);
	msleep(10);
	ret = i2c_master_send(client, send_reset, sizeof(send_reset));
	pinctrl_select_state(gpio->pinctrl, gpio->ven_low);
	msleep(10);

    if (ret != sizeof(send_reset)) 
	{
		pr_err("%s ret = %d\n", __func__, ret);
		return -ENODEV;
	}
	else
	{
		return 0;
	}
}

static ssize_t pn54x_show_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct PN54X_DEV *pn_dev = g_pn54x_dev;

	if (pn_dev == NULL)
	{
		printk("%s pn_dev NULL\n", __func__);
		return -EINVAL;
	}

	printk("%s\n", __func__);
	if (pn_dev->dev_exist)
	{
		len = snprintf(buf, PAGE_SIZE, "%s:%s:%s:%s\n", "pn547", "NXP", "sup1", "1");
	}
	else
	{
		len = snprintf(buf, PAGE_SIZE, "%s:%s:%s:%s\n", "NA", "NA", "NA", "NA");
	}

	return len;
}

static void pn54x_disable_irq(struct IRQ_INFO *irq)
{
	unsigned long flags;

	if (irq == NULL)
	{
		pr_err("%s irq NULL\n", __func__);
		return;
	}

	spin_lock_irqsave(&irq->lock, flags);
	if (irq->enable)
	{
       	disable_irq_nosync(irq->number);
		irq->enable = false;
	}
	spin_unlock_irqrestore(&irq->lock, flags);
}

static irqreturn_t pn54x_irq_handler(int irq, void *dev_id)
{
	struct PN54X_DEV *dev = g_pn54x_dev;

	if (dev == NULL)
	{
		pr_err("%s dev NULL\n", __func__);
		return IRQ_HANDLED;
	}

	if (!gpio_get_value(dev->irq.pin))
	{
		pr_err("%s bug\n", __func__);
		return IRQ_HANDLED;
	}

	pn54x_disable_irq(&dev->irq);
	wake_up(&dev->read_wq);
	return IRQ_HANDLED;
}

static irqreturn_t pn54x_clk_irq_handler(int irq, void *dev_id)
{
	struct PN54X_DEV *dev = g_pn54x_dev;

	if (dev == NULL)
	{
		pr_err("%s dev NULL\n", __func__);
		return IRQ_HANDLED;
	}

	pn54x_disable_irq(&dev->clk_irq);

	/* if nfc lock is active, unlock it */
	if (wake_lock_active(&dev->clk_lock))
	{
		printk("nfc wake_unlock\n");
		wake_unlock(&dev->clk_lock);
	}
	return IRQ_HANDLED;
}

static int pn54x_dev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = g_pn54x_dev;
	printk("pn54x %s : %d, %d\n", __func__, imajor(inode), iminor(inode));
	return 0;
}

static ssize_t pn54x_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0;
	int i   = 0;
	struct PN54X_DEV *dev = filp->private_data;

	if (dev == NULL)
	{
		printk("%s dev NULL\n", __func__);
		return -EINVAL;
	}

	if (count > MAX_BUFFER_SIZE)
	{
		count = MAX_BUFFER_SIZE;
	}

	printk("%s try to read %zu byte\n", __func__, count);

	mutex_lock(&dev->read_mutex);
	if (!gpio_get_value(dev->irq.pin))
	{
		printk("%s wait for event......\n", __func__);
		
		if (filp->f_flags & O_NONBLOCK)
		{
			ret = -EAGAIN;
			goto fail;
		}
		
		dev->irq.enable = true;
		enable_irq(dev->irq.number);
		ret = wait_event_interruptible(dev->read_wq, gpio_get_value(dev->irq.pin));
		pn54x_disable_irq(&dev->irq);

		if (ret)
		{
			printk("%s read wait event error\n", __func__);
			goto fail;
		}
	}

	/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
	/* if NFC need clk  */
	if (gpio_get_value(dev->clk_irq.pin))
	{
		if (wake_lock_active(&dev->clk_lock) == 0)
		{
			/* lock system to prevent from deep sleep */
			//wake_lock(&nfc_clk_lock);
			wake_lock_timeout(&dev->clk_lock, 3600 * HZ); /* it will auto unlock in 60min */

			if (dev->clk_irq.enable == false)
			{
				dev->clk_irq.enable = true;
				enable_irq(dev->clk_irq.number);
			}
		}
	}
	/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

	/* Read data */
    ret = i2c_master_recv(dev->client, (char *)(uintptr_t)dev->phy_dma_rd_buf, count);
	mutex_unlock(&dev->read_mutex);

	if (ret < 0)
	{
		pr_err("%s: i2c_master_recv failed, returned %d\n", __func__, ret);
		return -EBUSY;
	}

	if (ret > count)
	{
		pr_err("%s: received too many bytes from i2c %d\n", __func__, ret);
		return -EIO;
	}
	
    if (copy_to_user(buf, dev->virt_dma_rd_buf, ret)) 
    {
        pr_err(KERN_DEBUG "%s : failed to copy to user space\n", __func__);
        return -EFAULT;
    }
   
	printk("%s success IFD->PC: ", __func__);
	for(i = 0; i < ret; i++)
	{
		printk("%02X ", dev->virt_dma_rd_buf[i]);
	}
	printk("\n");

	return ret;

fail:
	printk("%s failed\n", __func__);
	mutex_unlock(&dev->read_mutex);
	return ret;
}

static ssize_t pn54x_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0;
	int i   = 0;
	int idx = 0;
	struct PN54X_DEV *dev = filp->private_data;

	if (dev == NULL)
	{
		printk("%s dev NULL\n", __func__);
		return -EINVAL;
	}

	if (count > MAX_BUFFER_SIZE)
	{
		count = MAX_BUFFER_SIZE;
	}

	printk("%s try to write %zu byte\n", __func__, count);

	if (copy_from_user(dev->virt_dma_wr_buf, &buf[idx * 255], count)) 
	{
		pr_err(KERN_DEBUG "%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	/* Write data */
    ret = i2c_master_send(dev->client, (char *)(uintptr_t)dev->phy_dma_wr_buf, count);
	if (ret != count)
	{
		pr_err("%s failed: i2c_master_send returned %d\n", __func__, ret);
		return -EIO;
	}

	printk("%s success PC->IFD: ", __func__);

	for (i = 0; i < count; i++)
	{
		printk("%02X ", dev->virt_dma_wr_buf[i]);
	}
	printk("\n");

	return ret;
}


static long pn54x_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct PN54X_DEV *dev = filp->private_data;

	if (dev == NULL)
	{
		printk("%s dev NULL\n", __func__);
		return -EINVAL;
	}

	switch (cmd)
	{
	case PN54X_SET_PWR:
		if (arg == 2)
		{
			printk("%s download firmware\n", __func__);
			clk_buf_ctrl(CLK_BUF_NFC, 1);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->ven_high);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->dl_high);
			msleep(10);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->ven_low);
			msleep(50);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->ven_high);
			msleep(10);
		}
		else if (arg == 1)
		{
			/* power on */
			printk("%s power on\n", __func__);
			clk_buf_ctrl(CLK_BUF_NFC, 1);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->dl_low);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->ven_high);
			msleep(10);
		}
		else if (arg == 0)
		{
			/* power off */
			printk("%s power off\n", __func__);
			clk_buf_ctrl(CLK_BUF_NFC, 0);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->dl_low);
			pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->ven_low);
			msleep(10);
		}
		else
		{
			pr_err("%s bad arg %lu\n", __func__, arg);
			return -EINVAL;
		}
		break;

	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

static int pn54x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret                  = 0;
	unsigned int ints[2]     = {0};
	struct device_node *node = NULL;
	struct PN54X_DEV *dev    = NULL;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}
	
	dev = kzalloc(sizeof(struct PN54X_DEV), GFP_KERNEL);
	if (dev == NULL)
	{
		pr_err("%s allocate memory for pn54x dev failed\n", __func__);
		return -ENOMEM;
	}

	g_pn54x_dev = dev;

	ret = pn54x_pinctrl_init(g_pn54x_platf_dev, &dev->gpio);
	if (ret != 0 || dev->gpio == NULL)
	{
		pr_err("%s pinctrl init failed\n", __func__);
		goto err_pinctrl_init;
	}

	if (pn54x_check_device(client, dev->gpio) == 0)
	{
		printk("%s : check device OK\n", __func__);
		dev->dev_exist = 1;
        
        #ifdef CONFIG_TCT_ROBUST
			nfc_init_flag = 1;
        #endif
	}
	else
	{
		pr_err("%s : no pn54x or it is bad\n", __func__);
		dev->dev_exist = 0;
		ret = -1;
		goto err_check_device;
	}

	dev->client    = client;
	client->timing = 400;

#ifdef CONFIG_MTK_I2C_EXTENSION
	client->addr     &= I2C_MASK_FLAG;
	client->ext_flag |= I2C_DMA_FLAG;

    #ifdef CONFIG_64BIT
	dev->virt_dma_wr_buf = dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE,
				                              &dev->phy_dma_wr_buf,
				                              GFP_KERNEL | GFP_DMA);
	dev->virt_dma_rd_buf = dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE,
				                              &dev->phy_dma_rd_buf,
				                              GFP_KERNEL | GFP_DMA);
    #else
	dev->virt_dma_wr_buf = dma_alloc_coherent(NULL, MAX_BUFFER_SIZE,
				                              &dev->phy_dma_wr_buf,
				                              GFP_KERNEL | GFP_DMA);
	dev->virt_dma_rd_buf = dma_alloc_coherent(NULL, MAX_BUFFER_SIZE,
				                              &dev->phy_dma_rd_buf,
				                              GFP_KERNEL | GFP_DMA);
    #endif

	if (dev->virt_dma_wr_buf == NULL || dev->virt_dma_rd_buf == NULL)
	{
		pr_err("%s dma buffer alloc failed\n", __func__);
		ret = -ENOMEM;
		goto err_dma_alloc;
	}
#endif

	init_waitqueue_head(&dev->read_wq);
	mutex_init(&dev->read_mutex);
	spin_lock_init(&dev->irq.lock);
	spin_lock_init(&dev->clk_irq.lock);
	wake_lock_init(&dev->clk_lock, WAKE_LOCK_SUSPEND, "nfc clock wakelock");

	dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
	dev->misc_dev.name  = PN54X_DEV_NAME;
	dev->misc_dev.fops  = &pn54x_dev_fops;

	ret = misc_register(&dev->misc_dev);
	if (ret)
	{
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}
	
	/* init irq pin and clk irq pin */
	pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->irq_cfg);
	pinctrl_select_state(dev->gpio->pinctrl, dev->gpio->clk_irq_cfg);

    node = of_find_compatible_node(NULL, NULL, "mediatek,nfc_irq");
	if (node)
	{
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		mt_gpio_set_debounce(ints[0], ints[1]);
		dev->irq.pin    = ints[0];
		dev->irq.number = irq_of_parse_and_map(node, 0);
        ret = request_irq(dev->irq.number,  pn54x_irq_handler, IRQF_TRIGGER_HIGH,
					      "pn54x-nfc-eint", NULL);
		if (ret)
		{
			pr_err("%s : requesting IRQ error\n", __func__);
			goto err_request_irq;
		} 
	}
	else
	{
		pr_err("%s : can not find NFC eint compatible node\n",  __func__);
		ret = -ENXIO;
		goto err_request_irq;
	}

	dev->irq.enable = true;
	pn54x_disable_irq(&dev->irq);

    node = of_find_compatible_node(NULL, NULL, "mediatek,nfc_clk_req_irq");
	if (node)
	{
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		mt_gpio_set_debounce(ints[0], ints[1]);
		dev->clk_irq.pin = ints[0];
		dev->clk_irq.number = irq_of_parse_and_map(node, 0);
        ret = request_irq(dev->clk_irq.number, pn54x_clk_irq_handler, IRQF_TRIGGER_LOW,
					      "pn54x-nfc-clk-eint", NULL);
		if (ret)
		{
			pr_err("%s : requesting CLK IRQ error\n", __func__);
			goto err_request_clk_irq;
		} 
	}
	else
	{
		pr_err("%s : can not find NFC clk eint compatible node\n",  __func__);
		ret = -ENXIO;
		goto err_request_clk_irq;
	}

	dev->clk_irq.enable = true;
	pn54x_disable_irq(&dev->clk_irq);
	i2c_set_clientdata(client, dev);
	printk("%s success\n", __func__);
	return 0;

err_request_clk_irq:
	free_irq(dev->irq.number, pn54x_irq_handler);
err_request_irq:
	misc_deregister(&dev->misc_dev);
err_misc_register:
    wake_lock_destroy(&dev->clk_lock);
	mutex_destroy(&dev->read_mutex);

#ifdef CONFIG_MTK_I2C_EXTENSION
err_dma_alloc:
	if (dev->virt_dma_wr_buf != NULL)
	{
        #ifdef CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, dev->virt_dma_wr_buf,
					      dev->phy_dma_wr_buf);
        #else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, dev->virt_dma_wr_buf,
					      dev->phy_dma_wr_buf);
        #endif
	}

	if (dev->virt_dma_rd_buf != NULL)
	{
        #ifdef CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, dev->virt_dma_rd_buf,
					      dev->phy_dma_rd_buf);
        #else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, dev->virt_dma_rd_buf,
					      dev->phy_dma_rd_buf);
        #endif
	}
#endif

	dev->client    = NULL;
	dev->dev_exist = 0;
err_check_device:
	dev->gpio->ven_high    = NULL;
	dev->gpio->ven_low     = NULL;
	dev->gpio->dl_high     = NULL;
	dev->gpio->dl_low      = NULL;
	dev->gpio->irq_cfg     = NULL;
	dev->gpio->clk_irq_cfg = NULL;
	devm_pinctrl_put(dev->gpio->pinctrl);
	dev->gpio->pinctrl = NULL;
	kfree(dev->gpio);
	dev->gpio = NULL;
err_pinctrl_init:
	kfree(dev);
	dev = NULL;
	g_pn54x_dev = NULL;
	g_pn54x_platf_dev = NULL;
	return ret;
}

static int pn54x_i2c_remove(struct i2c_client *client)
{
	struct PN54X_DEV *dev = i2c_get_clientdata(client);

	if (dev == NULL)
	{
		pr_err("%s dev NULL\n", __func__);
		return -EINVAL;
	}

	printk("%s\n", __func__);
	free_irq(dev->irq.number, NULL);
	free_irq(dev->clk_irq.number, NULL);
	misc_deregister(&dev->misc_dev);
	wake_lock_destroy(&dev->clk_lock);
	mutex_destroy(&dev->read_mutex);

#ifdef CONFIG_MTK_I2C_EXTENSION
    #ifdef CONFIG_64BIT
	dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, dev->virt_dma_wr_buf,
				      dev->phy_dma_wr_buf);
	dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, dev->virt_dma_rd_buf,
				      dev->phy_dma_rd_buf);
    #else
	dma_free_coherent(NULL, MAX_BUFFER_SIZE, dev->virt_dma_wr_buf,
				      dev->phy_dma_wr_buf);
	dma_free_coherent(NULL, MAX_BUFFER_SIZE, dev->virt_dma_rd_buf,
				      dev->phy_dma_rd_buf);
    #endif

	dev->virt_dma_wr_buf = NULL;
	dev->virt_dma_rd_buf = NULL;
#endif

	dev->client            = NULL;
	dev->gpio->ven_high    = NULL;
	dev->gpio->ven_low     = NULL;
	dev->gpio->dl_high     = NULL;
	dev->gpio->dl_low      = NULL;
	dev->gpio->irq_cfg     = NULL;
	dev->gpio->clk_irq_cfg = NULL;
	devm_pinctrl_put(dev->gpio->pinctrl);
	dev->gpio->pinctrl = NULL;
	kfree(dev->gpio);
	dev->gpio = NULL;
	kfree(dev);
	g_pn54x_dev = NULL;
	g_pn54x_platf_dev = NULL;
	return 0;
}

static int pn54x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	printk("%s\n", __func__);
	strcpy(info->type, PN54X_DEV_NAME);
	return 0;
}

static int pn54x_platform_probe(struct platform_device *pdev)
{
	g_pn54x_platf_dev = pdev;

	if (i2c_add_driver(&pn54x_i2c_driver) != 0)
    {
        pr_err("%s add pn54x_i2c_driver failed\n", __func__);
		g_pn54x_platf_dev = NULL;
        return -ENODEV;
    }	

	return 0;
}

static int pn54x_platform_remove(struct platform_device *pdev)
{
	i2c_del_driver(&pn54x_i2c_driver);
	return 0;
}

static int __init pn54x_init(void)
{
	if (platform_driver_register(&pn54x_platform_driver))
	{
		pr_err("%s platform_driver_register failed\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static void __exit pn54x_exit(void)
{
	platform_driver_unregister(&pn54x_platform_driver);
}

module_init(pn54x_init);
module_exit(pn54x_exit);
MODULE_LICENSE("GPL");
