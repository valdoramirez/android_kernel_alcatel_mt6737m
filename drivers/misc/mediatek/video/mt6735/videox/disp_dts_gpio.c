#include "disp_dts_gpio.h"
#include <linux/bug.h>

#include <linux/kernel.h> /* printk */

static struct pinctrl *this_pctrl; /* static pinctrl instance */

/* DTS state mapping name */
static const char *this_state_name[DTS_GPIO_STATE_MAX] = {
	"mode_te_gpio",                 /* DTS_GPIO_STATE_TE_MODE_GPIO */
	"mode_te_te",                   /* DTS_GPIO_STATE_TE_MODE_TE */
//	"pwm_test_pin_mux_gpio55",      /* DTS_GPIO_STATE_PWM_TEST_PINMUX_55 */
//	"pwm_test_pin_mux_gpio69",      /* DTS_GPIO_STATE_PWM_TEST_PINMUX_69 */
//	"pwm_test_pin_mux_gpio129",     /* DTS_GPIO_STATE_PWM_TEST_PINMUX_129 */
					/*add by llf for LCD bias begain*/
	"lcd_bias_enp0",
	"lcd_bias_enp1",
	"lcd_bias_enn0",
	"lcd_bias_enn1",
	"lcd_bias_vci0", 		/* DTS_GPIO_STATE_LCD_BIAS_VCI0,*/
	"lcd_bias_vci1",		/* DTS_GPIO_STATE_LCD_BIAS_VCI1,*/
	"lcd_bias_vdd0",		/* DTS_GPIO_STATE_LCD_BIAS_VDD0,*/
	"lcd_bias_vdd1"		/* DTS_GPIO_STATE_LCD_BIAS_VDD1,*/
					/*add by llf for LCD bias end*/				
};

/* pinctrl implementation */
static long _set_state(const char *name)
{
	long ret = 0;
	struct pinctrl_state *pState = 0;

	BUG_ON(!this_pctrl);

	pState = pinctrl_lookup_state(this_pctrl, name);
	if (IS_ERR(pState)) {
		pr_err("set state '%s' failed\n", name);
		ret = PTR_ERR(pState);
		goto exit;
	}
	printk("[LLF] the pin name is %s \n",name);
	/* select state! */
	pinctrl_select_state(this_pctrl, pState);

exit:
	return ret; /* Good! */
}

long disp_dts_gpio_init(struct platform_device *pdev)
{
	long ret = 0;
	struct pinctrl *pctrl;

	/* retrieve */
	printk("[LLF] pinctl inited OK!\n");
	pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pctrl)) {
		dev_err(&pdev->dev, "Cannot find disp pinctrl!");
		ret = PTR_ERR(pctrl);
		goto exit;
	}

	this_pctrl = pctrl;

exit:
	return ret;
}

long disp_dts_gpio_select_state(DTS_GPIO_STATE s)
{
	BUG_ON(!((unsigned int)(s) < (unsigned int)(DTS_GPIO_STATE_MAX)));
	return _set_state(this_state_name[s]);
}

