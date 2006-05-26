TOP=../..

include $(TOP)/configure/CONFIG

HOST_OPT = NO

PROD = streamApp
DBD += streamApp.dbd

PROD_SRCS += streamApp_registerRecordDeviceDriver.cpp
PROD_SRCS_DEFAULT += streamAppMain.cc
PROD_SRCS_vxWorks += -nil-

PROD_LIBS_DEFAULT += stream
PROD_LIBS_vxWorks += -nil-
PROD_OBJS_vxWorks += $(INSTALL_BIN)/streamLibrary

PROD_LIBS += asyn
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

#Write StreamDevice debug output to this file
CPPFLAGS += -DDEBUGFILE=StreamDebug.log

include $(TOP)/configure/RULES

clean::
	rm -f core*

build: link

link:
	rm -f ../streamApp
	ln -s O.$(EPICS_HOST_ARCH)/streamApp ../streamApp
