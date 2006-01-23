TOP=../..

include $(TOP)/configure/CONFIG

-include CONFIG_STREAM
-include ../CONFIG_STREAM

HOST_OPT = NO
CFLAGS += -Werror
CXXFLAGS += -Werror

LOADABLE_LIBRARY = stream

DBD += stream.dbd

LIB_SRCS += stream_registerRecordDeviceDriver.cpp
LIB_SRCS += $(STREAM_SRCS)
LIB_SRCS += $(RECORDS:%=dev%Stream.c)
LIB_SRCS += $(BUSSES:%=%Interface.cc)
LIB_SRCS += $(FORMATS:%=%Converter.c)

# use memguard
#CPPFLAGS += -include ../memguard.h
#LIB_SRCS += memguard.cc

INC += devStream.h

include $(TOP)/configure/RULES

# update version string (contains __DATE__ and __TIME__)
# each time make runs
StreamVersion.o: FORCE
FORCE:
