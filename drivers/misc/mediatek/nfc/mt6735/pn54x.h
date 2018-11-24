#ifndef _PN54X_H_
#define _PN54X_H_

#define PN54X_DEV_NAME  "pn544"
#define PN54X_MAGIC     0xE9
#define PN54X_SET_PWR   _IOW(PN54X_MAGIC, 0x01, unsigned int)
#define MAX_BUFFER_SIZE 512
#define PN54X_I2C_ADDR  0x28

struct IRQ_INFO
{
	bool         enable;
	unsigned int number;
	unsigned int pin;
	spinlock_t   lock;
};

struct PIN_CTRL
{
	struct pinctrl *pinctrl;
	struct pinctrl_state *ven_high;
	struct pinctrl_state *ven_low;
	struct pinctrl_state *dl_high;
	struct pinctrl_state *dl_low;
	struct pinctrl_state *irq_cfg;
	struct pinctrl_state *clk_irq_cfg;
};

struct PN54X_DEV 
{
	bool              dev_exist;
#ifdef CONFIG_MTK_I2C_EXTENSION
	char              *virt_dma_wr_buf;
	char              *virt_dma_rd_buf;
	dma_addr_t        phy_dma_wr_buf;
	dma_addr_t        phy_dma_rd_buf;
#endif
	struct IRQ_INFO   irq;
	struct IRQ_INFO   clk_irq;
	wait_queue_head_t read_wq;
	struct wake_lock  clk_lock;
	struct mutex      read_mutex;
	struct PIN_CTRL   *gpio;
	struct i2c_client *client;
	struct miscdevice misc_dev;
};

static int pn54x_platform_probe(struct platform_device *pdev);
static int pn54x_platform_remove(struct platform_device *pdev);
static int pn54x_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int pn54x_i2c_remove(struct i2c_client *client);
static int pn54x_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int pn54x_dev_open(struct inode *inode, struct file *filp);
static ssize_t pn54x_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset);
static ssize_t pn54x_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset);
static long pn54x_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static ssize_t pn54x_show_info(struct device *dev, struct device_attribute *attr, char *buf);
extern int mt_gpio_set_debounce(unsigned gpio, unsigned debounce);
extern unsigned int irq_of_parse_and_map(struct device_node *dev, int index);

#endif
