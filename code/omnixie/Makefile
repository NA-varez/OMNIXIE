all : flash

BUILD_DIR:=build
SRC:=omnixie.c

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(SRC): $(SRC) | $(BUILD_DIR)
	ln -sf ../$(SRC) $@

TARGET:=$(BUILD_DIR)/omnixie

ADDITIONAL_C_FILES+=../rv003usb/rv003usb/rv003usb.S ../rv003usb/rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../rv003usb/lib -I../rv003usb/rv003usb -I.

TARGET_MCU?=CH32V003
include ../rv003usb/ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean


