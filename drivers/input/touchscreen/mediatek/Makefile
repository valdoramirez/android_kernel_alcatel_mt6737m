subdir-ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek
# In case the platform does NOT support this type of sensors
obj-y   +=  mtk_tpd.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_init.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o

# add by rongxiao.deng porting shine lite CTP driver
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_SHINELITE)	+=  shine_lite_GT615/
# add end
# add by rongxiao.deng porting shine lite CTP driver
obj-$(CONFIG_TOUCHSCREEN_MTK_SHINELITE_MSG2840)	+=  shine_lite_msg2840/
# add end
# add by rongxiao.deng porting shine 4g GT1151 CTP driver
obj-$(CONFIG_TOUCHSCREEN_MTK_GT1151_SHINE4G)   +=  shine4g_GT1151/
# add end
obj-$(CONFIG_TOUCHSCREEN_MTK_GT910)	+=  GT910/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XXTB_HOTKNOT)	+=  GT9XXTB_hotknot/
# add by rongxiao.deng porting shine4g CTP driver
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_SHINE4G)	+=  shine4g_GT9XXTB_hotknot/
# add end
obj-$(CONFIG_TOUCHSCREEN_MTK_SYNAPTICS_I2C_RMI4)	+=  synaptics_i2c_rmi4/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT928)	+=  GT928/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_HOTKNOT_SCP)	+=  GT9XX_hotknot_scp/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT911)	+=  GT911/
obj-$(CONFIG_TOUCHSCREEN_MTK_FT5X0X)	+=  ft5x0x/
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG2238)	+=  MSG2238/
obj-$(CONFIG_TOUCHSCREEN_MTK_FT6436)	+=  ft6436/
