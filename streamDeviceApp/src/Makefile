TOP=../..

ifdef EPICS_HOST_ARCH
# R3.14 Makefile

include $(TOP)/configure/CONFIG

USR_CFLAGS += -DABOVE_EPICS_R3_13
USR_CFLAGS += -Werror

# -------------------------------
# Build an Diamond Support Module
# -------------------------------

LIBRARY_IOC_vxWorks += streamDevice

INSTALL_DBDS += $(INSTALL_DBD)/stream.dbd


# Support definitions
RECORDS += Ao Ai
RECORDS += Bo Bi
RECORDS += Mbbo Mbbi
RECORDS += MbboDirect MbbiDirect
RECORDS += Lo Li
RECORDS += So Si
RECORDS += Wf


# The following are compiled and added to the support library
streamDevice_SRCS += devStream.c
streamDevice_SRCS += devStreamProtocol.c
streamDevice_SRCS += $(foreach RECORD, $(RECORDS), dev$(RECORD)Stream.c)
streamDevice_SRCS += streamTty.c

# The following adds support from base/src/vxWorks
streamDevice_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# We need to link against the EPICS Base libraries
streamDevice_LIBS += $(EPICS_BASE_IOC_LIBS)


include $(TOP)/configure/RULES
#----------------------------------------

# ukp: 	This is the hack that generates the dbd file from
#		two source .dbd files (to be R3.13 backwards compatible).
stream.dbd: streamDriver.dbd streamRegistrar.dbd
	cat $^ > $@


else
# R3.13 Makefile


include $(TOP)/config/CONFIG_APP
include $(TOP)/config/RULES_ARCHS

endif
