/*************************************************************************************************
hi553_otp.c
---------------------------------------------------------
OTP Application file From Huaquan for hi553
2015.08.19
---------------------------------------------------------
NOTE:
The modification is appended to initialization of image sensor. 
After sensor initialization, use the function , and get the id value.
bool otp_wb_update(BYTE zone)
and
bool otp_lenc_update(BYTE zone), 
then the calibration of AWB and LSC will be applied. 
After finishing the OTP written, we will provide you the golden_rg and golden_bg settings.
**************************************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>  
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "hi553mipiraw_Sensor.h"

#define PFXO "HI553OTP"
#define LOG_INFO(format, args...)	pr_debug(PFXO "[%s] " format, __FUNCTION__, ##args)
#define hi553MIPI_WRITE_ID   0x40

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);//add by hhl
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);//add by hhl


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

#define HQ_ID         	   0x45
//#define LARGAN_LENS        0x00
//#define DONGWOON           0x01
//#define TDK_VCM			 0x00
#define HQ_VALID_OTP       0x01
#define HQ_ID_OFFSET       0x11
#define HQ_WB_OFFSET       0x1E

//#define GAIN_DEFAULT       0x0100

USHORT HQ_Golden_RG = 355;   
USHORT HQ_Golden_BG = 295;

USHORT HQ_Current_RG;
USHORT HQ_Current_BG;


//kal_uint32 r_ratio;
//kal_uint32 b_ratio;


static void hi553_write_cmos_sensor(kal_uint16 addr, kal_uint8 para)
{
    //kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, hi553MIPI_WRITE_ID);
}


kal_uint16 hi553_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,hi553MIPI_WRITE_ID);
    return get_byte;
}
/*************************************************************************************************
* Function    :  hi553_start_read_otp
* Description :  before read otp , set the reading block setting  
* Parameters  :  void
* Return      :  void
**************************************************************************************************/
void hi553_start_read_otp(void)
{
	hi553_write_cmos_sensor(0x0a02, 0x01); // Fast sleep on
	hi553_write_cmos_sensor(0x0a00, 0x00); // stand by on
	Sleep(20);
	hi553_write_cmos_sensor(0x0f02, 0x00); // pll disable
	hi553_write_cmos_sensor(0x011a, 0x01); // CP TRIM_H
	hi553_write_cmos_sensor(0x011b, 0x09); // IPGM TRIM_H
	hi553_write_cmos_sensor(0x0d04, 0x01); // Fsync(OTP busy) Output Enable
	hi553_write_cmos_sensor(0x0d00, 0x07); // Fsync(OTP busy) Output Drivability
	hi553_write_cmos_sensor(0x003f, 0x10); // OTP R/W mode
	hi553_write_cmos_sensor(0x0a00, 0x01); // stand by off
}

/*************************************************************************************************
* Function    :  hi553_stop_read_otp
* Description :  after read otp , stop and reset otp block setting  
**************************************************************************************************/
void hi553_stop_read_otp(void)
{
	hi553_write_cmos_sensor(0x0a00, 0x00); // stand by on
	Sleep(20);
	hi553_write_cmos_sensor(0x003f, 0x00); // display mode
	hi553_write_cmos_sensor(0x0a00, 0x01); // stand by off
}


/*************************************************************************************************
* Function    :  hi553_get_otp_flag
* Description :  get otp WRITTEN_FLAG  
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0B
* Return      :  [BYTE], if 1 , this type has valid otp data, otherwise, invalid otp data
**************************************************************************************************/
BYTE hi553_get_otp_flag(BYTE zone)
{
	BYTE flag = 0;
	hi553_start_read_otp();
	if(zone==0)
	{
		hi553_write_cmos_sensor(0x10a, (0x0501 >> 8) & 0xff); // start address H
		hi553_write_cmos_sensor(0x10b, 0x0501 & 0xff); // start address L
		hi553_write_cmos_sensor(0x102, 0x01); // single read
		flag = hi553_read_cmos_sensor(0x0108);	
	}
	if(zone==1)
	{
		hi553_write_cmos_sensor(0x10a, (0x0535 >> 8) & 0xff); // start address H
		hi553_write_cmos_sensor(0x10b, 0x0535 & 0xff); // start address L
		hi553_write_cmos_sensor(0x102, 0x01); // single read
		flag = hi553_read_cmos_sensor(0x0108);
	}
	if(zone==2)
	{
		hi553_write_cmos_sensor(0x10a, (0x0590 >> 8) & 0xff); // start address H
		hi553_write_cmos_sensor(0x10b, 0x0590 & 0xff); // start address L
		hi553_write_cmos_sensor(0x102, 0x01); // single read
		flag = hi553_read_cmos_sensor(0x0108);	
	}
	hi553_stop_read_otp();
	LOG_INFO("hi553otpFlag:0x%02x",flag );
    return flag;
	/*
	if(flag==0x00)//empty
	{
		return 0;
		
	}
	if(flag==0x01 || flag==0x13 || flag==0x37)//valid
	{
		return 1;
		
	}
	*/
}

/*************************************************************************************************
* Function    :  hi553_get_otp_date
* Description :  get otp date value    
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0B    
**************************************************************************************************/
bool hi553_get_otp_date(BYTE zone) 
{
	BYTE year  = 0;
	BYTE month = 0;
	BYTE day   = 0;
    hi553_start_read_otp();

	hi553_write_cmos_sensor(0x10a, ((0x0504+(zone*HQ_ID_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0504+(zone*HQ_ID_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	year  = hi553_read_cmos_sensor(0x0108);
    
	hi553_write_cmos_sensor(0x10a, ((0x0505+(zone*HQ_ID_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0505+(zone*HQ_ID_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	month = hi553_read_cmos_sensor(0x0108);
    
	hi553_write_cmos_sensor(0x10a, ((0x0506+(zone*HQ_ID_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0506+(zone*HQ_ID_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	day   = hi553_read_cmos_sensor(0x0108);

	hi553_stop_read_otp();

    LOG_INFO("OTP date=%02d.%02d.%02d", year,month,day);

	return 1;
}


/*************************************************************************************************
* Function    :  hi553_get_otp_module_id
* Description :  get otp MID value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0B
* Return      :  [BYTE] 0 : OTP data fail 
                 other value : module ID data , HUAQUAN ID is 0x0002            
**************************************************************************************************/
BYTE hi553_get_otp_module_id(BYTE zone)
{
	BYTE module_id = 0;
    hi553_start_read_otp();

	hi553_write_cmos_sensor(0x10a, ((0x0502+(zone*HQ_ID_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0502+(zone*HQ_ID_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	module_id = hi553_read_cmos_sensor(0x0108);	
	Sleep(4);
	 
	hi553_stop_read_otp();

	LOG_INFO("OTP_Module ID: 0x%02x.\n",module_id);

	return module_id;
}


/*************************************************************************************************
* Function    :  hi553_get_otp_lens_id
* Description :  get otp LENS_ID value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0B
* Return      :  [BYTE] 0 : OTP data fail 
                 other value : LENS ID data             
**************************************************************************************************/
BYTE hi553_get_otp_lens_id(BYTE zone)
{
	BYTE lens_id = 0;

    hi553_start_read_otp();

	hi553_write_cmos_sensor(0x10a, ((0x0508+(zone*HQ_ID_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0508+(zone*HQ_ID_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	lens_id = hi553_read_cmos_sensor(0x0108);
  
	hi553_stop_read_otp();

	LOG_INFO("OTP_Lens ID: 0x%02x.\n",lens_id);

	return lens_id;
}


/*************************************************************************************************
* Function    :  hi553_wb_gain_set
* Description :  Set WB ratio to register gain setting  512x
* Parameters  :  [int] r_ratio : R ratio data compared with golden module R
                       b_ratio : B ratio data compared with golden module B
* Return      :  [bool] 0 : set wb fail 
                        1 : WB set success            
**************************************************************************************************/

bool hi553_wb_gain_set(void)
{
/*
	USHORT R_GAIN=4,B_GAIN=4,G_GAIN=4;

	R_GAIN = 4*(HQ_Golden_RG / HQ_Current_RG);
	B_GAIN = 4*(HQ_Golden_BG / HQ_Current_BG);  
	 
	if(R_GAIN<B_GAIN)   
	{  
		if(R_GAIN<4)   
		{  
			B_GAIN=B_GAIN/R_GAIN;  
			G_GAIN=G_GAIN/R_GAIN;  
			R_GAIN=4;  
		}  
	}  
	else  
	{  
		if(B_GAIN<4)   
		{  
			R_GAIN=R_GAIN/B_GAIN;  
			G_GAIN=G_GAIN/B_GAIN;  
			B_GAIN=4;  
		}  
	}

	LOG_INFO("OTP_HQ_Golden_RG=%d,HQ_Golden_BG=%d\n",HQ_Golden_RG,HQ_Golden_BG);
	LOG_INFO("OTP_HQ_Current_RG=%d,HQ_Current_BG=%d\n",HQ_Current_RG,HQ_Current_BG);
	LOG_INFO("wangxiaoyu OTP_gain    R_Gain=%d B_Gain=%d G_Gain=%d\n",  R_GAIN,B_GAIN,G_GAIN);  
 
	hi553_write_cmos_sensor(0x0500, (BYTE)(G_GAIN)>>8); 
	hi553_write_cmos_sensor(0x0501, (BYTE)(G_GAIN)&0xff);
	hi553_write_cmos_sensor(0x0502, (BYTE)(G_GAIN)>>8); 
	hi553_write_cmos_sensor(0x0503, (BYTE)(G_GAIN)&0xff); 
	 
	hi553_write_cmos_sensor(0x0504, (BYTE)(R_GAIN)>>8);
	hi553_write_cmos_sensor(0x0505, (BYTE)(R_GAIN)&0xff); 
	hi553_write_cmos_sensor(0x0506, (BYTE)(B_GAIN)>>8); 
	hi553_write_cmos_sensor(0x0507, (BYTE)(B_GAIN)&0xff);
*/
	LOG_INFO("OTP WB Update Finished! \n");
	return 1;
	
}

/*************************************************************************************************
* Function    :  hi553_get_otp_wb
* Description :  Get WB data    
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f      
**************************************************************************************************/
bool hi553_get_otp_wb(int zone)
{

	BYTE temph = 0;
	BYTE templ = 0;
	
	hi553_start_read_otp();

	hi553_write_cmos_sensor(0x10a, ((0x0536+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0536+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	temph = hi553_read_cmos_sensor(0x0108);	
	hi553_write_cmos_sensor(0x10a, ((0x0537+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0537+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	templ = hi553_read_cmos_sensor(0x0108);
	HQ_Current_RG = (USHORT)((temph<<8)| templ);

	hi553_write_cmos_sensor(0x10a, ((0x0538+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0538+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	temph = hi553_read_cmos_sensor(0x0108);	
	hi553_write_cmos_sensor(0x10a, ((0x0539+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x0539+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	templ = hi553_read_cmos_sensor(0x0108);
	HQ_Current_BG = (USHORT)((temph<<8)| templ);
/*
	hi553_write_cmos_sensor(0x10a, ((0x053C+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x053C+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	temph = hi553_read_cmos_sensor(0x0108);	
	hi553_write_cmos_sensor(0x10a, ((0x053D+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x053D+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	templ = hi553_read_cmos_sensor(0x0108);
	HQ_Golden_RG = (USHORT)((temph<<8)| templ);

	hi553_write_cmos_sensor(0x10a, ((0x053E + (zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x053E + (zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	temph = hi553_read_cmos_sensor(0x0108);	
	hi553_write_cmos_sensor(0x10a, ((0x053F+(zone*HQ_WB_OFFSET)) >> 8) & 0xff); // start address H
	hi553_write_cmos_sensor(0x10b, (0x053F+(zone*HQ_WB_OFFSET)) & 0xff); // start address L
	hi553_write_cmos_sensor(0x102, 0x01); // single read
	templ = hi553_read_cmos_sensor(0x0108);
	HQ_Golden_BG = (USHORT)((temph<<8)| templ);
*/
	hi553_stop_read_otp();

	return 1;
}


/*************************************************************************************************
* Function    :  hi553_otp_wb_update
* Description :  Update WB correction 
* Return      :  [bool] 0 : OTP data fail 
                        1 : otp_WB update success            
**************************************************************************************************/
bool hi553_otp_wb_update(BYTE zone)
{
	//USHORT golden_g, current_g;


	if(!hi553_get_otp_wb(zone))  // get wb data from otp
	{
		return 0;
	}	
	//r_ratio = 512 * HQ_Golden_RG / HQ_Current_RG;
	//b_ratio = 512 * HQ_Golden_BG / HQ_Current_BG;

	return 1;
}

/*************************************************************************************************
* Function    :  hi553_otp_update()
* Description :  update otp data from otp , it otp data is valid, 
                 it include get ID and WB update function  
* Return      :  [bool] 0 : update fail
                        1 : update success
**************************************************************************************************/
bool hi553_otp_update(void)
{
	BYTE zone = 0x00;
    BYTE zone1 = 0x00;
	BYTE FLG = 0x00;
	BYTE MID = 0x00,LENS_ID= 0x00;
	int i;
	
	for(i=0;i<3;i++)
	{
		FLG = hi553_get_otp_flag(0);
		if(FLG != 0)
			break;
	}
	if(i==3)
	{
		LOG_INFO("liuchrg_Warning: No OTP Data or OTP data is invalid!!");
		return 0;
	}
    if(FLG == 0x1)
    {
        zone = 0;
    }
    else if(FLG == 0x13)
    {
        zone = 1;
    }
    else if(FLG == 0x37)
    {
        zone = 2;
    }
    else
    {
		LOG_INFO("liuchrg_Warning: No OTP Data or OTP data is invalid!!");
		return 0;
	}
	
		MID = hi553_get_otp_module_id(zone);
		LENS_ID = hi553_get_otp_lens_id(zone);
	  	hi553_get_otp_date(zone);
	  	
	if(MID != HQ_ID)
	{
		LOG_INFO("liuchrg_Warning: No SEASON Module !!!!");
		return 0;
	}
//------------------------------awb---------------------------
	for(i=0;i<3;i++)
	{
		FLG = hi553_get_otp_flag(1);
		if(FLG != 0)
			break;
	}
	if(i==3)
	{
		LOG_INFO("liuchrg_Warning: No awb OTP Data or OTP data is invalid!!");
		return 0;
	}
    if(FLG == 0x1)
    {
        zone1 = 0;
    }
    else if(FLG == 0x13)
    {
        zone1 = 1;
    }
    else if(FLG == 0x37)
    {
        zone1 = 2;
    }
    else
    {
		LOG_INFO("liuchrg_Warning: No awb OTP Data or OTP data is invalid!!");
		return 0;
    }
	hi553_otp_wb_update(zone1);	
	LOG_INFO("WB read finished! \n");
return 1;
	
}
