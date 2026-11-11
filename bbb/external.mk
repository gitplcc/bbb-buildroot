TARGET_BOARD_NAME = $(subst _defconfig,,$(notdir $(BR2_DEFCONFIG)))
include $(sort $(wildcard $(BR2_EXTERNAL_BBB_PATH)/package/*/*.mk))
