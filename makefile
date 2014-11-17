#
#             LUFA Library
#     Copyright (C) Dean Camera, 2014.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

## --- for matrixstorm's avrstick with atxmega128a3u
MCU          = atxmega128a3u
ARCH         = XMEGA
BOARD        = USER
F_CPU        = 32000000
F_USB        = 48000000
## --- for bobricius' usb stick with m32u4
#MCU          = atmega32u4
#ARCH         = AVR8
#BOARD        = OLIMEX32U4
#F_CPU        = 16000000
#F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = enstix
# $(shell find "crypto/avr-crypto-lib/aes" -name "*.c" -o -name "*.S") $(shell find "crypto/avr-crypto-lib/bcal/" -name "bcal_aes*.c" -o -name "bcal-basic.c" -o -name "bcal-cbc.c" -o -name "*.S") $(shell find "crypto/avr-crypto-lib/memxor" -name "*.c" -o -name "*.S")
SRC          = $(TARGET).c LufaLayer.c Descriptors.c Timer.c SCSI/SCSI.c VirtualFAT/VirtualFAT.c $(shell find "crypto" -maxdepth 1 -name "*.c" -o -name "*.S") $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/
LD_FLAGS     = apipage.a

CC_FLAGS    += -DFORMATTED_DATE=\"`date +%Y-%m-%d@%H:%M`\"

# Default target
all:

# flabbergast: add default avrdude settings
AVRDUDE_PROGRAMMER = avr109
#AVRDUDE_PORT = /dev/tty.usbmodem004581
AVRDUDE_PORT = /dev/ttyACM0
#AVRDUDE_PORT = /dev/tty.usbmodemfa131

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
