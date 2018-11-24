#ifndef _LCM_DEFINE_H
#define _LCM_DEFINE_H

/* LCM_FUNC */
#define LCM_FUNC_GPIO	1
#define LCM_FUNC_I2C	2
#define LCM_FUNC_UTIL	3
#define LCM_FUNC_CMD	4

/* LCM_GPIO_TYPE */
#define LCM_GPIO_MODE	1
#define LCM_GPIO_DIR	2
#define LCM_GPIO_OUT	3

/* LCM_GPIO_MODE_DATA */
#define LCM_GPIO_MODE_00	0
#define LCM_GPIO_MODE_01	1
#define LCM_GPIO_MODE_02	2
#define LCM_GPIO_MODE_03	3
#define LCM_GPIO_MODE_04	4
#define LCM_GPIO_MODE_05	5
#define LCM_GPIO_MODE_06	6
#define LCM_GPIO_MODE_07	7
#define MAX_LCM_GPIO_MODE	8

/* LCM_GPIO_DIR_DATA */
#define LCM_GPIO_DIR_IN	0
#define LCM_GPIO_DIR_OUT	1

/* LCM_GPIO_OUT_DATA */
#define LCM_GPIO_OUT_ZERO	0
#define LCM_GPIO_OUT_ONE	1

/* LCM_I2C_TYPE */
#define LCM_I2C_WRITE	1

/* LCM_UTIL_TYPE */
#define LCM_UTIL_RESET	1
#define LCM_UTIL_MDELAY	2
#define LCM_UTIL_UDELAY	3
#define LCM_UTIL_WRITE_CMD_V1	4
#define LCM_UTIL_WRITE_CMD_V2	5
#define LCM_UTIL_READ_CMD_V1	6
#define LCM_UTIL_READ_CMD_V2	7
#define LCM_UTIL_RAR	8
//Begin add by xiaobin.zhai for lcd device tree
#define LCM_FUN_ID_CHECK      11
#define LCM_FUN_ID_SET      12
//End add by xiaobin.zhai for lcd device tree
/* LCM_UTIL_RESET_DATA */
#define LCM_UTIL_RESET_LOW	0
#define LCM_UTIL_RESET_HIGH	1

/* LCM_UTIL_WRITE_CMD_V2_DATA */
#define LCM_UTIL_WRITE_CMD_V2_NULL	0xF9
//Begin add by xiaobin.zhai for lcd device tree
#define LCM_GPIO_ID1_MODE     0
#define LCM_GPIO_ID2_MODE     1
//End add by xiaobin.zhai for lcd device tree

#endif				/* _LCM_DEFINE_H */
