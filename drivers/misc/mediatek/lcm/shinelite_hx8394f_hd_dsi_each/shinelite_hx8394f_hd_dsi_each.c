
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

#define FRAME_WIDTH  					(720)
#define FRAME_HEIGHT 					(1280) 

#define REGFLAG_DELAY             				0xFD
#define REGFLAG_END_OF_TABLE      				0xFE   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE						0


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define LCM_EACH			0

bool lcm_hx8394f_vendor=LCM_EACH;	//default to choose byd panel
extern int tps65132_write_bytes(unsigned char addr, unsigned char value);


//set LCM IC ID
#define LCM_ID_HX8394F 	(0x839400)
#define LCD_VCC_EN      (GPIO43 | 0x80000000)
#define LCD_VCC_EP      (GPIO42 | 0x80000000)
#define GPIO_LCM_ID1	(GPIO81 | 0x80000000)
#define GPIO_LCM_ID2	(GPIO82 | 0x80000000)

//#define GPIO_LCM_ID1	GPIO81_LCD_ID1
//#define GPIO_LCM_ID2	GPIO82_LCD_ID2

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 									(lcm_util.udelay(n))
#define MDELAY(n) 									(lcm_util.mdelay(n))

#ifdef BUILD_LK
#define LCD_DEBUG(fmt, args...) printf(fmt, ##args)		
#else
#define LCD_DEBUG(fmt, args...) printk(fmt, ##args)
#endif
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)						lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   					lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define set_gpio_lcd_enp(cmd) \
	lcm_util.set_gpio_lcd_enp_bias(cmd)
#define set_gpio_lcd_enn(cmd) \
	lcm_util.set_gpio_lcd_enn_bias(cmd)

struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};



static struct LCM_setting_table lcm_each_initialization_setting[] = {

	/* Set EXTC */	
	{0xb9, 3,  { 0xff, 0x83, 0x94 }},
		
	/* Set Power */  
	{0xb1, 10, { 0x50, 0x15, 0x75, 0x09, 0x32, 
		      0x44, 0x71, 0x31, 0x55, 0x2f }},//5n  p2 3  0x10    16 // ea y 95 y   11   f1   4e4 595
			                  
	/* Set MIPI */	
	{0xba, 6,  { 0x63, 0x03, 0x68, 0x6b, 0xb2,0xc0 }},

	/* Set D2 */   
	{0xd2, 1,  { 0x88 }},//n  Set D2 VSPR UP to 5V  44  ****  33/4.9 44/5.0   66/5.2
	
	/* Set Display */	
	{0xb2, 5,  { 0x00, 0x80, 0x64, 0x10, 0x07 }},
			       	              
	/* Set CYC */   
	{0xb4, 21, { 0x01, 0x74, 0x01, 0x74, 0x01,
		      0x74, 0x01, 0x0c, 0x86, 0x75,           
		      0x00, 0x3f, 0x01, 0x74, 0x01,             
		      0x74, 0x01, 0x74, 0x01, 0x0c,           
		      0x86 }},
			      	
	/* Set D3 */	
	{0xd3, 33, { 0x00, 0x00, 0x07, 0x07, 0x40,
		      0x1e, 0x08, 0x00, 0x32, 0x10,          
		      0x08, 0x00, 0x08, 0x54, 0x15,        
		      0x10, 0x05, 0x04, 0x02, 0x12,          
		      0x10, 0x05, 0x07, 0x23, 0x23,          
		      0x0c, 0x0c, 0x27, 0x10, 0x07,           
		      0x07, 0x10, 0x40}},	

	/* Set GIP */	
	{0xd5, 42, { 0x19, 0x19, 0x18, 0x18, 0x1b,
		      0x1b, 0x1a, 0x1a, 0x04, 0x05,         
		      0x06, 0x07, 0x00, 0x01, 0x02,
		      0x03, 0x20, 0x21, 0x18, 0x18,   
		      0x22, 0x23, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x18,   
		      0x18, 0x18, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x18,        
		      0x18, 0x18, 0x18, 0x18 }},
	
	/* Set D6 */	
	{0xd6, 44, { 0x18, 0x18, 0x19, 0x19, 0x1b,
		      0x1b, 0x1a, 0x1a, 0x03, 0x02,             
		      0x01, 0x00, 0x07, 0x06, 0x05,
		      0x04, 0x23, 0x22, 0x18, 0x18,         
		      0x21, 0x20, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x18,         
		      0x18, 0x18, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x18,        
		      0x18, 0x18, 0x18, 0x18 }},// Set D6

	/* Set Gamma */ 
	{0xe0, 58, { 0x00, 0x0b,0x17,0x1e,0x20,
		      0x24,0x28,0x26,0x4e,0x5d,             
		      0x6d,0x6b,0x74,0x84,0x89,		    
		      0x8e,0x9a,0x9b,0x96,0xa4,		    
		      0xB2,0x58,0x55,0x59,0x5b,		    
		      0x5d,0x60,0x64,0x7F,0x00,
		      0x0b,0x17,0x1d,0x20,0x24,
		      0x28,0x26,0x4e,0x5d,0x6d,
		      0x6b,0x74,0x85,0x8a,0x8e,
		      0x9a,0x9b,0x97,0xa5,0xB2,
		      0x58,0x55,0x58,0x5b,0x5d,
		      0x61,0x65,0x7F}},//n Set Gamma  2.2   150408

	 /* Set panel */  
	{0xcc, 1, { 0x0b }},   // Set Panel 9  03
			      
	 /* Set c0 */ 	
	{0xc0, 2, { 0x1f, 0x31 }},// Set C0 73
	
	 /* Set vcom */   
	{0xb6, 2, { 0x80, 0x80}},//  Set VCOM  3f  96 57
	 
	 /* Set d4 */    
	{0xd4, 1, { 0x02 }},  
	{0xbd, 1, { 0x01 }},// Set BD  
	{0xb1, 1, { 0x00 }},// Set GAS 
	{0xbd, 1, { 0x01 }},// Set BD
	
	{0xb0, 0, {0x80}},//bist mode open
	{0x11, 0, {0x00}},//sleep out
	
	/* Set Display */	
	{0xb2, 12, { 0x00, 0x80, 0x64, 0x10, 0x07, 
		      0x2f, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x18 }},
	{REGFLAG_DELAY, 150, {}},
	{0x29,1,{0x00}},//display on 
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {

	{0xB9, 3, {0xff,0x83,0x94}},// Change to Page 0

	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 150, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {

	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 150, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY :
		MDELAY(table[i].count);
		break;

		case REGFLAG_END_OF_TABLE :
		break;

		default:
		dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	//LCD_DEBUG("\t\t hx8394f [lcm_set_util_funcs]\n");

	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	//LCD_DEBUG("\t\t hx8394f [lcm_get_params]\n");

	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	//params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
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

	params->dsi.word_count=480*3;

	//here is for esd protect by legen
	//params->dsi.noncont_clock = true;
	//params->dsi.noncont_clock_period=2;
	params->dsi.lcm_ext_te_enable=false;
	//for esd protest end by legen

	//delete by zhuqiang 2013.3.4 
	//params->dsi.word_count=FRAME_WIDTH*3;	
	// add by zhuqiang for FR437058 at 2013.4.25 begin
	params->dsi.vertical_sync_active=6;  //4
	params->dsi.vertical_backporch=14;	//16
	params->dsi.vertical_frontporch=20;
	// add by zhuqiang for FR437058 at 2013.4.25 end
	params->dsi.vertical_active_line=FRAME_HEIGHT;

	//delete by zhuqiang 2013.3.4 
	//params->dsi.line_byte=2180;		
	// add by zhuqiang for FR437058 at 2013.4.25 begin
	params->dsi.horizontal_sync_active=10;  

	//zrl modify for improve the TP reort point begin,130916
	params->dsi.horizontal_backporch=80;   //50  60 
	params->dsi.horizontal_frontporch=80;  //50   200
	//zrl modify for improve the TP reort point end,130916

	// add by zhuqiang for FR437058 at 2013.4.25 end
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;	//added by zhuqiang 2013.3.4 

	params->dsi.HS_TRAIL= 7;  // 4.3406868
	params->dsi.HS_PRPR = 4;
	
	params->dsi.CLK_TRAIL= 50;
 /*
	params->dsi.lcm_esd_check_table[1].cmd          = 0x00;
	params->dsi.lcm_esd_check_table[1].count        = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x21;  //page6

	params->dsi.lcm_esd_check_table[2].cmd          = 0x3d;
	params->dsi.lcm_esd_check_table[2].count        = 1;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x22; //page6

	// add by zhuqiang for FR437058 at 2013.4.25 begin
	//params->dsi.pll_div1=1;         //  div1=0,1,2,3;  div1_real=1,2,4,4
	//params->dsi.pll_div2=1;         // div2=0,1,2,3;div2_real=1,2,4,4

	//zrl modify for improve the TP reort point begin,130916
	//params->dsi.fbk_div =30;              // fref=26MHz,  fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)   //32
	//zrl modify for improve the TP reort point end,130916
*/
	// add by zhuqiang for FR437058 at 2013.4.25 end
 	params->dsi.PLL_CLOCK=208;//add by longfang.liu PR527476
        //params->dsi.ssc_range = 7;//add by llf for 431570
        //LCD_DEBUG(KERN_ERR "[shinelite_hx8394f_hd_dsi_each.c]Debug MIPI: params->dsi.PLL_CLOCK = %d\n", params->dsi.PLL_CLOCK);
        //LCD_DEBUG(KERN_ERR "[shinelite_hx8394f_hd_dsi_each.c]Debug MIPI: params->dsi.ssc_range = %d\n", params->dsi.ssc_range);
}

//legen add for detect lcm vendor
#if 0
static bool lcm_select_panel(void)
{
	int value=0;

	//LCD_DEBUG("\t\t hx8394f [lcm_select_panel]\n");

	mt_set_gpio_mode(GPIO_LCM_ID1,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);
	mt_set_gpio_mode(GPIO_LCM_ID2,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID2, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID2, GPIO_PULL_DISABLE);

	MDELAY(10);
	value = mt_get_gpio_in(GPIO_LCM_ID1)<<1 | mt_get_gpio_in(GPIO_LCM_ID2);
	if(value)
		return value;

	return LCM_EACH;
}
#endif
//legen add end 

//static int first_init=0;
static void lcm_init(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret=0;
	cmd=0x00;
	data=0x0E;
	//VPS=0x00;data=0x0A;VSP=5V,
	//         data=0x0E;VSP=5.4V,
	//VNG=0x01;data=0x0A;VNG=-5V,
	//         data=0x0E;VNG=-5.4V,
#ifndef CONFIG_FPGA_EARLY_PORTING
	//enable power
	//lcm_util.set_gpio_out(LCD_VCC_EP, GPIO_OUT_ZERO);
	//lcm_util.set_gpio_out(LCD_VCC_EN, GPIO_OUT_ZERO);
	set_gpio_lcd_enp(0);
	set_gpio_lcd_enn(0);
	MDELAY(50);

	//lcm_util.set_gpio_out(LCD_VCC_EP, GPIO_OUT_ONE);
	set_gpio_lcd_enp(1);
	MDELAY(10);

	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		LCD_DEBUG("[KERNEL]LM3463-----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		LCD_DEBUG("[KERNEL]LM3463-----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

	MDELAY(10);
	//lcm_util.set_gpio_out(LCD_VCC_EN, GPIO_OUT_ONE);
	set_gpio_lcd_enn(1);
	MDELAY(10);
	cmd=0x01;
	data=0x0E;
	//VPS=0x00;data=0x0A;VSP=5V,
	//         data=0x0E;VSP=5.4V,
	//VNG=0x01;data=0x0A;VNG=-5V,
	//         data=0x0E;VNG=-5.4V,

	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		LCD_DEBUG("[KERNEL]LM3463-----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		LCD_DEBUG("[KERNEL]LM3463-----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

#endif

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_each_initialization_setting, sizeof(lcm_each_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

	SET_RESET_PIN(0); 
	MDELAY(20);
	//disable power
	//mt_set_gpio_out(LCD_VCC_EN, GPIO_OUT_ONE);
	//mt_set_gpio_out(LCD_VCC_EN, GPIO_OUT_ZERO);
	set_gpio_lcd_enn(1);
	set_gpio_lcd_enn(0);
	MDELAY(10);
	//mt_set_gpio_out(LCD_VCC_EP, GPIO_OUT_ONE);
	//mt_set_gpio_out(LCD_VCC_EP, GPIO_OUT_ZERO);
	set_gpio_lcd_enp(1);
	set_gpio_lcd_enp(0);
	MDELAY(50);
}

static void lcm_resume(void)
{
	lcm_init();  
	MDELAY(20);
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
#if 0
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
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

	//LCD_DEBUG("\t\t hx8394f [lcm_update]\n");
	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK

	unsigned char buffer[4];
	unsigned int array[16];

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("hx8394f: lcm_esd_check enter\n");
#endif

	array[0]=0x00063902;
	array[1]=0x0698ffff;
	array[2]=0x00000004;
	dsi_set_cmdq(array, 3, 1);
	MDELAY(10);

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x0A, buffer, 1);

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("hx8394f: lcm_esd_check  0x0A = %x\n",buffer[0]);
#endif

	if(buffer[0] != 0x9C)
	{
		return 1;
	}
	else
	{
		return 0 ;
	}

#if 0
	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0A, &buffer[0], 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0C, &buffer[1], 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0D, &buffer[2], 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array,1,1);
	read_reg_v2(0x0E, &buffer[3], 1);

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("hx8394f: lcm_esd_check  0x09(bit0~3) = %x \n",buffer[0]);
#endif

	//if ((buffer[0]==0x80)&&(buffer[1]==0x73)&&(buffer[2]==0x04)&&(buffer[3]==0x00))
	if ((buffer[0]==0x0)&&(buffer[1]==0x70)&&(buffer[2]==0x0)&&(buffer[3]==0x00))
	{
#if defined(LCM_ESD_DEBUG)
		//LCD_DEBUG("hx8394f: lcm_esd_check exit\n");
#endif
		return 0;
	}
	else
	{
		return 0 ;
	}
#endif	

#endif
}


static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK

#if defined(LCM_ESD_DEBUG)
	LCD_DEBUG("hx8394f: lcm_esd_recover enter");
#endif

	lcm_init();
	return 1;

#endif 
}


// ---------------------------------------------------------------------------
//  Get LCM ID Information
// ---------------------------------------------------------------------------

static unsigned int lcm_compare_id(void)
{
	int id_type=0;	

	mt_set_gpio_mode(GPIO_LCM_ID1,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
	//mt_set_gpio_pull_select(GPIO_LCM_ID1,GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);// def 0
	
	mt_set_gpio_mode(GPIO_LCM_ID2,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_ID2, GPIO_DIR_IN);
	//mt_set_gpio_pull_select(GPIO_LCM_ID2,GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO_LCM_ID2, GPIO_PULL_DISABLE);//def 0

	MDELAY(10);
	id_type = mt_get_gpio_in(GPIO_LCM_ID1)<<1 | mt_get_gpio_in(GPIO_LCM_ID2) ;
	LCD_DEBUG("[LLF] hx8394f [lcm_compare_id   id_type=%d ]\n" , id_type);


	if (id_type == 1 ) //hx8394f_EACH  source ,and ID_tpye is 1. 
	{
		return 1 ;
	}
	else
	{
		return  0 ;
	}
} 

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER shinelite_hx8394f_hd_dsi_each_lcm_drv =
{
	.name		= "shinelite_hx8394f_hd_dsi_each",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if 0
	.update         = lcm_update,
#endif

	.esd_check   = lcm_esd_check,
	.esd_recover   = lcm_esd_recover,
	.compare_id    = lcm_compare_id,
};
