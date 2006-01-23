TOP=../..

include $(TOP)/configure/CONFIG

PROD_HOST = streamApp
DBD += streamApp.dbd

PROD_SRCS += streamApp_registerRecordDeviceDriver.cpp
PROD_SRCS_DEFAULT += streamAppMain.cc
PROD_SRCS_vxWorks += -nil-

PROD_LIBS += stream
PROD_LIBS += asyn

#Write StreamDevice debug output to this file
CPPFLAGS += -DDEBUGFILE=StreamDebug.log

include $(TOP)/configure/RULES
