#ifndef BUILD_LK
    #include <linux/string.h>
    #include <linux/kernel.h>
#endif
#include "lcm_drv.h"
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>	
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
    #include <platform/mt_pmic.h>
#else
    #include <mt-plat/mt_gpio.h>
    #include <mach/gpio_const.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)
#define GPIO_LCD_RST_PIN 	(GPIO146 | 0x80000000)   //GPIO28   ->reset
#define GPIO_LCM_VCI_EN_PIN 	(GPIO21 | 0x80000000)   //VGP3-> VCI 3v      gpio108 ldo->LCD 3V->vci 3v
#define GPIO_LCM_VDD_EN_PIN 	(GPIO2 | 0x80000000)    //VIO1.8-> VDD 1.8v      gpio102 ldo->LCD1.8V->vdd3 1.8v
#define LCM_DSI_CMD_MODE	0
#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifdef BUILD_LK
#define	LCM_DEBUG(format, ...)   printf("lk " format "\n", ## __VA_ARGS__)
#elif defined(BUILD_UBOOT)
#define	LCM_DEBUG(format, ...)   printf("uboot " format "\n", ## __VA_ARGS__)
#else
#define	LCM_DEBUG(format, ...)   printk("kernel " format "\n", ## __VA_ARGS__)
#endif


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 		(lcm_util.udelay(n))
#define MDELAY(n)		(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        	lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(ppdata, queue_size, force_update)		lcm_util.dsi_set_cmdq (ppdata, queue_size, force_update)
#define wrtie_cmd(cmd)						lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)			lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)						lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size) 

#define set_gpio_lcd_vci(cmd) \
	lcm_util.set_gpio_lcd_vci_bias(cmd)
#define set_gpio_lcd_vdd(cmd) \
	lcm_util.set_gpio_lcd_vdd_bias(cmd)
int amoled_set_backlight(int level);
struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};
/*
static void init_lcm_registers(void)
{
	unsigned int data_array[16];                  

	data_array[0] = 0x00033902;		// Common Setting 
	data_array[1] = 0x005A5AF0;
	dsi_set_cmdq(&data_array[0], 2, 1);	

	data_array[0] = 0x00033902;		
	data_array[1] = 0x005A5AFC;
	dsi_set_cmdq(&data_array[0], 2, 1);

	data_array[0] = 0x00043902;		
	data_array[1] = 0x20D8D8C0;
	dsi_set_cmdq(&data_array[0], 2, 1);

	data_array[0] = 0x00053902;		
	data_array[1] = 0x000516FD;
	data_array[1] = 0x00000020;
	dsi_set_cmdq(&data_array[0], 3, 1);

	data_array[0] = 0x00033902;		
	data_array[1] = 0x00A5A5F0;
	dsi_set_cmdq(&data_array[0], 2, 1);

	data_array[0] = 0x00033902;		
	data_array[1] = 0x00A5A5FC;
	dsi_set_cmdq(&data_array[0], 2, 1);

	//data_array[0] = 0x20531500;   //Dimming Speed 1 frame
	//dsi_set_cmdq(&data_array[0], 1, 1);
	//data_array[0] = 0x00551500; 	//>1 Open CABC
	//dsi_set_cmdq(&data_array[0], 1, 1);

	MDELAY(20);				//Star Display on
	data_array[0] = 0x00110500;		
	dsi_set_cmdq(&data_array[0], 1, 1);
	MDELAY(50);
	data_array[0] = 0x42511500; 	//Brightness Control level=102:0x42
	dsi_set_cmdq(&data_array[0], 1, 1);
	MDELAY(150);
	data_array[0] = 0x00290500;		  // Display On
	dsi_set_cmdq(&data_array[0], 1, 1);

}
*/
static void init_lcm_registers(void)
{
	unsigned int data_array[16];                  

	data_array[0] = 0x00110500;             // Sleep Out
	dsi_set_cmdq(&data_array[0], 1, 1);
	MDELAY(30);

	data_array[0] = 0x00033902;      //Common
	data_array[1] = 0x005A5AF0;
	dsi_set_cmdq(&data_array[0], 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x20D8D8C0;
	dsi_set_cmdq(&data_array[0], 2, 1);

	//data_array[0] = 0x00053902;		
	//data_array[1] = 0x000516FD;
	//data_array[1] = 0x00000020;
	//dsi_set_cmdq(&data_array[0], 3, 1);

	data_array[0] = 0x00033902;           
	data_array[1] = 0x00A5A5F0;
	dsi_set_cmdq(&data_array[0], 2, 1);      

	data_array[0] = 0x20531500;         //Brightness Control
	dsi_set_cmdq(&data_array[0], 1, 1);
	  
	data_array[0] = 0x00511500;
	dsi_set_cmdq(&data_array[0], 1, 1);

	MDELAY(200);
	data_array[0] = 0x00290500;         // Display On
	dsi_set_cmdq(&data_array[0], 1, 1);

}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	//params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY; 
	//					LCM_DBI_TE_MODE_VSYNC_OR_HSYNC;                                                
	params->dbi.te_mode                 = LCM_DBI_TE_MODE_DISABLED;
	//params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

	#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   =  CMD_MODE;
	#else
	params->dsi.mode   = BURST_VDO_MODE;//ESYNC_PULSE_VDO_MODE;//; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
	#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   		= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     		= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      		= LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	params->dsi.packet_size=256;

	// Video mode setting	

	// add by zhuqiang for FR437058 at 2013.4.25 begin
	params->dsi.intermediat_buffer_num = 2;	
	// add by zhuqiang for FR437058 at 2013.4.25 end
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.word_count=720*3;

	//here is for esd protect by legen
	//params->dsi.noncont_clock = true;
	//params->dsi.noncont_clock_period=2;
	params->dsi.lcm_ext_te_enable=false;
	//for esd protest end by legen

	params->dsi.vertical_sync_active				= 2;// 0x05
	params->dsi.vertical_backporch					= 8;//  0x0d
	params->dsi.vertical_frontporch					= 14; // 0x08
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 4;// 0x12
	params->dsi.horizontal_backporch				= 50;//0x5f
	params->dsi.horizontal_frontporch				= 52;//0x5f
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	//params->dsi.LPX=8; 
	params->dsi.cont_clock= 1;

	params->dsi.esd_check_enable = 1;

	params->dsi.customization_esd_check_enable = 1;

	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;  //page0
/*	params->dsi.lcm_esd_check_table[1].cmd          = 0x04;
	params->dsi.lcm_esd_check_table[1].count        = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x32;  //page0
	params->dsi.lcm_esd_check_table[2].cmd          = 0x05;
	params->dsi.lcm_esd_check_table[2].count        = 1;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x00;  //page0
*/
	// Bit rate calculation
	params->dsi.PLL_CLOCK = 208;
	//1 Every lane speed
	//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
	//params->dsi.fbk_div =18;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

}

static void lcm_reset(void)
{
	//mt_set_gpio_mode(GPIO_LCD_RST_PIN, GPIO_MODE_00);
	//mt_set_gpio_dir(GPIO_LCD_RST_PIN, GPIO_DIR_OUT);        
	//mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	SET_RESET_PIN(1);
	MDELAY(20); // 1ms
	
	//mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
	SET_RESET_PIN(0);
	MDELAY(5); // 1ms

	//mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
	SET_RESET_PIN(1);
	MDELAY(50); // 10ms
}


static void lcm_init(void)
{
        //VGP3-> VCI 3v      
	set_gpio_lcd_vci(0);
	MDELAY(10); // 1ms
	set_gpio_lcd_vci(1);
	
	//VGP3-> VDD 1.8v 
	set_gpio_lcd_vdd(0);
	MDELAY(10);
	set_gpio_lcd_vdd(0);

    	LCM_DEBUG("------shine4g_s6e8aa5_hd_dsi_vdo----%s------\n",__func__);
	lcm_reset();
	MDELAY(10); // 1ms
	init_lcm_registers();
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;		//Begain Display Off
	dsi_set_cmdq(&data_array[0], 1, 1);
	MDELAY(10);
	
	data_array[0] = 0x00100500;		// Display Off
	dsi_set_cmdq(&data_array[0], 1, 1);
	MDELAY(150);

	//mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);//Reset Off
	SET_RESET_PIN(0);
	MDELAY(10);

	//mt_set_gpio_out(GPIO_LCM_VCI_EN_PIN, GPIO_OUT_ONE);//Off VCI
	set_gpio_lcd_vci(0);
	MDELAY(10);

	//mt_set_gpio_out(GPIO_LCM_VDD_EN_PIN, GPIO_OUT_ZERO);//Off VDD
	set_gpio_lcd_vdd(1);
}


static void lcm_resume(void)
{
    	LCM_DEBUG("---[kernel]---shine4g_s6e8aa5_hd_dsi_vdo resume----%s------\n",__func__);
	lcm_init();
	//amoled_set_backlight(1024);
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(&data_array[0], 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(&data_array[0], 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(&data_array[0], 1, 0);

}
#endif
/*
static struct LCM_setting_table bl_level[] = {
       {0x51, 1, {0xFF} },
       {REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{

       LCM_LOGI("%s,nt35695 backlight: level = %d\n", __func__, level);

       bl_level[0].para_list[0] = level;

       push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}
*/

static struct LCM_setting_table lcm_level_setting={0x51, 1, {0xFF}};
static void lcm_set_backlight(unsigned int level)                                                                             
{
	unsigned char mapped_level = 0;
	//unsigned int data_array[16];
	
	//for LGE backlight IC mapping table
	if(level > 0) {
		mapped_level = (unsigned char)(level>>2);// 1024level >>2 ==>256level
		if(mapped_level<1)
			mapped_level=1;
	}else{
		mapped_level=0;
	}
	MDELAY(2); // 1ms
	//set_gpio_lcd_vdd(0);
	LCM_DEBUG("------lcm_set_backlight 1024level: %d  ==>256level: %d\n",level,mapped_level);
	//data_array[0] = (0x00511500 | (mapped_level<<24));		// Display On
	//dsi_set_cmdq(&data_array[0], 1, 1);
	lcm_level_setting.para_list[0]=mapped_level;
	if(level>0)
		dsi_set_cmdq_V2(lcm_level_setting.cmd, lcm_level_setting.count, lcm_level_setting.para_list, 1);
}

int amoled_set_backlight(int level)//define the level = 1024 mines old level;
{
	static int old_level=1023;
	if(level==1024){
		LCM_DEBUG("------lcm_set_backlight old level: %d\n",old_level);
		lcm_set_backlight(old_level);
	}else{
		lcm_set_backlight(level);
	}
	if(level>0 && level<1024){
		old_level=level;
	}
	return 0;
}
/*
//extern int IMM_GetOneChannelValue(int dwChannel, int deCount);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
#define VOLTAGE_FULL_RANGE_LCD 	1500 // VA voltage
#define ADC_PRECISE 		4096 // 12 bits
static unsigned int lcm_compare_id(void)
{
		return 1;
	int data[4] = {0,0,0,0};
	int res =0;
	int rawdata=0;
	int val=0;

	res =IMM_GetOneChannelValue(0,data,&rawdata);

	if(res<0) {
#ifdef BUILD_LK
		printf("shine4g_s6e8aa5_hd_dsi_vdo ,adc err= %d\n",res);  
#else
		printk("shine4g_s6e8aa5_hd_dsi_vdo ,adc err= %d\n",res);  
#endif
		return 0; 
	} else {
		val=rawdata*VOLTAGE_FULL_RANGE_LCD/ADC_PRECISE;
#ifdef BUILD_LK
		printf("shine4g_s6e8aa5_hd_dsi_vdo  chip_id  read value: %d\n",  val);
#else
		printk("shine4g_s6e8aa5_hd_dsi_vdo  chip_id  read value: %d\n",  val);
#endif
	}
	if(val < 100)
		return 1;
	else
		return 0;
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[2];
	int   array[4];

	/// please notice: the max return packet size is 1
	/// if you want to change it, you can refer to the following marked code
	/// but read_reg currently only support read no more than 4 bytes....
	/// if you need to read more, please let BinHan knows.
	
	//   unsigned int data_array[16];
	//   unsigned int max_return_size = 1;

	//   data_array[0]= 0x00003700 | (max_return_size << 16);    

	//   dsi_set_cmdq(&data_array, 1, 1);
	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);
	if(buffer[0]==0x1C)
	{
		return FALSE;
	}
	else
	{            
		return TRUE;
	}                                                                                                                        
#endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	lcm_resume();

	return TRUE;
}
*/


LCM_DRIVER shine4g_s6e8aa5_hd_dsi_vdo_lcm_drv = 
{
	.name		= "shine4g_s6e8aa5_hd_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.set_backlight	= lcm_set_backlight,
	//.compare_id     = lcm_compare_id,
	//.esd_check = lcm_esd_check,
	//.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};

