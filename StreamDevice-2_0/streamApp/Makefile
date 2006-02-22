TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


#=============================
# build an ioc application

PROD_IOC = streamApp
# streamApp.dbd will be created and installed
DBD += streamApp.dbd

# stream.dbd will be made up from these files:
##stream_DBD += base.dbd
##stream_DBD += xxxSupport.dbd
##stream_DBD += dbSubExample.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
streamApp_SRCS += streamApp_registerRecordDeviceDriver.cpp
streamApp_SRCS_DEFAULT += streamAppMain.cpp
streamApp_SRCS_vxWorks += -nil-

# Add locally compiled object code
##stream_SRCS += dbSubExample.c

# load streamLibrary.o for vxWorks
streamApp_OBJS_vxWorks += $(TOP)/bin/$(T_A)/streamLibrary

# The following adds support from base/src/vxWorks
streamApp_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

streamApp_LIBS += stream
streamApp_LIBS += asyn

#Write StreamDevice debug output to this file
CPPFLAGS += -DDEBUGFILE=StreamDebug.log

##streamApp_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

