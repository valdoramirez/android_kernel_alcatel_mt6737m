#ifndef N2DM_H
#define N2DM_H
	 
#include <linux/ioctl.h>

extern struct acc_hw* n2dm_get_cust_acc_hw(void) ;

#define N2DM_I2C_SLAVE_ADDR		0x10//0x10<-> SD0=GND;   0x12<-> SD0=High
	 
	 /* N2DM Register Map  (Please refer to N2DM Specifications) */
#define N2DM_REG_CTL_REG1		0x20
#define N2DM_REG_CTL_REG2		0x21
#define N2DM_REG_CTL_REG3		0x22
#define N2DM_REG_CTL_REG4		0x23
#define N2DM_REG_DATAX0		    0x28
#define N2DM_REG_OUT_X		    0x28
#define N2DM_REG_OUT_Y		    0x2A
#define N2DM_REG_OUT_Z		    0x2C


#define N2DM_REG_DEVID			0x0F //added by lanying.he 


#define N2DM_FIXED_DEVID			0x33
	 
#define N2DM_BW_200HZ			0x60
#define N2DM_BW_100HZ			0x50 //400 or 100 on other choise //changed
#define N2DM_BW_50HZ				0x40

#define	N2DM_FULLRANG_LSB		0XFF
	 
#define N2DM_MEASURE_MODE		0x08	//changed 
#define N2DM_DATA_READY			0x07    //changed
	 
//#define N2DM_FULL_RES			0x08
#define N2DM_RANGE_2G			0x00
#define N2DM_RANGE_4G			0x10
#define N2DM_RANGE_8G			0x20 //8g or 2g no ohter choise//changed
//#define N2DM_RANGE_16G			0x30 //8g or 2g no ohter choise//changed

#define N2DM_SELF_TEST           0x10 //changed
	 
#define N2DM_STREAM_MODE			0x80
#define N2DM_SAMPLES_15			0x0F
	 
#define N2DM_FS_8G_LSB_G			0x20
#define N2DM_FS_4G_LSB_G			0x10
#define N2DM_FS_2G_LSB_G			0x00
	 
#define N2DM_LEFT_JUSTIFY		0x04
#define N2DM_RIGHT_JUSTIFY		0x00
	 
	 
#define N2DM_SUCCESS						0
#define N2DM_ERR_I2C						-1
#define N2DM_ERR_STATUS					-3
#define N2DM_ERR_SETUP_FAILURE			-4
#define N2DM_ERR_GETGSENSORDATA			-5
#define N2DM_ERR_IDENTIFICATION			-6
	 
	 
	 
#define N2DM_BUFSIZE				256
	 
#endif

