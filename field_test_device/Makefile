APPLICATION=field-test-device

.PHONY: all
all:
	$(info $$DS75LX is ${DS75LX})
	$(info $$GPS is ${GPS})
	$(info $$DEVEUI is ${DEVEUI})
	$(info $$DEVADDR is ${DEVADDR})
	$(info $$OPERATOR is ${OPERATOR})

include ./Makefile.git

include ./Makefile.device

BOARD ?= im880b

ifeq ($(BOARD),im880b)
DS75LX ?= 1
GPS ?= 1
endif

ifeq ($(DS75LX),1)
USEMODULE += ds75lx
CFLAGS += -DDS75LX=1
endif

ifeq ($(GPS),1)
CFLAGS += -DGPS=1
# define the GNSS module baudrate
CFLAGS += -DSTD_BAUDRATE=9600
endif

FEATURES_REQUIRED += periph_rtc
FEATURES_REQUIRED += periph_wdt

USEMODULE += fmt
USEMODULE += xtimer
USEMODULE += hashes

# Default radio driver is Semtech SX1272 (used by the IMST iM880ab boards)
DRIVER ?= sx1272

LORA_DRIVER ?= $(DRIVER)
LORA_REGION ?= $(REGION)

# Default region is Europe and default band is 868MHz
REGION ?= EU868
#REGION ?= EU433

USEPKG += semtech-loramac
USEMODULE += $(DRIVER)
USEMODULE += semtech_loramac_rx
#
# DRPWSZ_SEQUENCE contains the sequence of triplets <datarate,tx power,payload size>
# If datarate is 255, the ADR is set to TRUE
#


# By default (for all except LLCC68)
ifndef LORAMAC_JOIN_MIN_DATARATE
ifeq ($(DRIVER),LLCC68) 
LORAMAC_JOIN_MIN_DATARATE ?= 1
else
LORAMAC_JOIN_MIN_DATARATE ?= 0
endif
endif

# By default
ifndef DRPWSZ_SEQUENCE
ifeq ($(DRIVER),LLCC68) 
# For LLCC68 (DR1 to DR5 @ 14 dBm)
DRPWSZ_SEQUENCE ?= 1,14,8,1,14,32,1,14,16,2,14,16,3,14,16,4,14,16,5,14,16,5,11,16,5,8,16,5,5,16,5,2,16
else
DRPWSZ_SEQUENCE ?= 0,14,8,0,14,32,0,14,16,1,14,16,2,14,16,3,14,16,4,14,16,5,14,16,5,11,16,5,8,16,5,5,16,5,2,16
endif
endif

# By default
ifndef TXPERIOD
TXPERIOD ?= 30
endif

# By default
ifndef TXCNF
TXCNF ?= false
endif

# initial ADR
ADR_ON ?= false

MIN_PORT ?= 1
MAX_PORT ?= 170


DEVELHELP ?= 1

ifeq ($(OTAA),1)
CFLAGS += -DOTAA=1
else
CFLAGS += -DOTAA=0
endif

ifeq ($(OTAA),0)
CFLAGS += -DDEVADDR=\"$(DEVADDR)\"
CFLAGS += -DAPPSKEY=\"$(APPSKEY)\"
CFLAGS += -DNWKSKEY=\"$(NWKSKEY)\"
endif

ifdef LABEL
CFLAGS += -DLABEL=\"$(LABEL)\"
endif

ifndef SECRET
# SECRET should be changed and kept secret
SECRET ?= cafebabe02000001cafebabe02ffffff
endif

ifndef DEVEUI
CFLAGS += -DFORGE_DEVEUI_APPEUI_APPKEY -DSECRET=\"$(SECRET)\"
else
CFLAGS += -DDEVEUI=\"$(DEVEUI)\" -DAPPEUI=\"$(APPEUI)\" -DAPPKEY=\"$(APPKEY)\"
endif

ifndef OPERATOR
OPERATOR ?= Undefined
endif

CFLAGS += -DREGION_$(REGION)
CFLAGS += -DLORAMAC_REGION_STR=\"$(REGION)\"
#CFLAGS += -DLORAMAC_ACTIVE_REGION=LORAMAC_REGION_$(REGION)
CFLAGS += -DLORAMAC_JOIN_MIN_DATARATE=$(LORAMAC_JOIN_MIN_DATARATE)
CFLAGS += -DDRPWSZ_SEQUENCE=$(DRPWSZ_SEQUENCE)
CFLAGS += -DTXPERIOD=$(TXPERIOD)
CFLAGS += -DTXCNF=$(TXCNF)
CFLAGS += -DADR_ON=$(ADR_ON)
CFLAGS += -DMIN_PORT=$(MIN_PORT) -DMAX_PORT=$(MAX_PORT)

CFLAGS += -DOPERATOR=\"$(OPERATOR)\"

# Send a APP_TIME_REQ every 100 messages
CFLAGS += -DAPP_TIME_REQ_PERIOD=100


#CFLAGS += -DVIRT_DEV=\"$(VIRT_DEV)\"


include $(RIOTBASE)/Makefile.include
