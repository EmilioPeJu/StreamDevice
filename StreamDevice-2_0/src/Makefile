TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library

-include CONFIG_STREAM
-include ../CONFIG_STREAM

HOST_OPT = NO
#CFLAGS += -Werror
CXXFLAGS += -Werror


LIBRARY_IOC += stream

# install stream.dbd into <top>/dbd
DBD += stream.dbd

# The following are compiled and added to the Support library
stream_SRCS += stream_registerRecordDeviceDriver.cpp
stream_SRCS += $(STREAM_SRCS)
stream_SRCS += $(RECORDS:%=dev%Stream.c)
stream_SRCS += $(BUSSES:%=%Interface.cc)
stream_SRCS += $(FORMATS:%=%Converter.c)

OBJS_vxWorks += $(BUSSES:%=%Interface.o)
OBJS_vxWorks += $(FORMATS:%=%Converter.o)

##xxxSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# use memguard
#CPPFLAGS += -include ../memguard.h
#LIB_SRCS += memguard.cc

INC += devStream.h

#=============================


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# update version string (contains __DATE__ and __TIME__)
# each time make runs
StreamVersion.o: FORCE
FORCE:
