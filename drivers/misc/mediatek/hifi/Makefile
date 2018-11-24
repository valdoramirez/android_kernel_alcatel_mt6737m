
TMP_CFLAGS := -I$(src)/../common
ccflags-y  += $(TMP_CFLAGS)

ifeq ($(strip $(CONFIG_ARCH_MTK_PROJECT)),"shine_plus")
#obj-$(CONFIG_SHINE_PLUS_HIFI_AK4376) += shineplus-ak4376/
endif

obj-$(CONFIG_MTK_HIFI) += $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))/

