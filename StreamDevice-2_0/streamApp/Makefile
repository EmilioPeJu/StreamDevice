TOP=../..

include $(TOP)/configure/CONFIG

PROD_IOC = streamApp
DBD += streamApp.dbd

streamApp_SRCS += streamApp_registerRecordDeviceDriver.cpp
streamApp_SRCS_DEFAULT += streamAppMain.cpp
streamApp_SRCS_vxWorks += -nil-

# load streamLibrary.o for vxWorks
streamApp_OBJS_vxWorks += $(TOP)/bin/$(T_A)/streamLibrary

# The following adds support from base/src/vxWorks
streamApp_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

streamApp_LIBS += stream
streamApp_LIBS += asyn

#Write StreamDevice debug output to this file
CPPFLAGS += -DDEBUGFILE=StreamDebug.log

include $(TOP)/configure/RULES

