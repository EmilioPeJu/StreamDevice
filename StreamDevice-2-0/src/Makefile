TOP=../..

include $(TOP)/configure/CONFIG

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

# Build vxWorks streamLibrary.o
# (vxWorks linker seems to remove these from stream.o because they are not referenced. 
#  Until we think of a better solution we will build a vxWorkes combined object 
#  library or the missing bits)
OBJLIB_vxWorks += stream
OBJLIB_SRCS += $(BUSSES:%=%Interface.cc)
OBJLIB_SRCS += $(FORMATS:%=%Converter.c)

# use memguard
#CPPFLAGS += -include ../memguard.h
#LIB_SRCS += memguard.cc

INC += devStream.h
#INC += memguard.h
INC += StreamBuffer.h
INC += StreamBusInterface.h
INC += StreamCore.h
INC += StreamError.h
INC += StreamFormatConverter.h
INC += StreamFormat.h
INC += StreamProtocol.h

include $(TOP)/configure/RULES

# update version string (contains __DATE__ and __TIME__)
# each time make runs
StreamVersion.o: FORCE
FORCE:
