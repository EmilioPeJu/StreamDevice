TOP=../..

include $(TOP)/configure/CONFIG

-include CONFIG_STREAM
-include ../CONFIG_STREAM

# For vxWorks, we need an OBJLIB.
# Otherwise bus and converter registry does not work.
LIBRARY_DEFAULT = stream
LIBRARY_vxWorks = -nil-
OBJLIB_vxWorks = $(LIBRARY_DEFAULT)
OBJLIB_SRCS = $(SRCS)

DBD += $(LIBRARY_DEFAULT).dbd

ifdef ASYN_VERS
LIB_LIBS += asyn

ifeq ($(ASYN_COMPAT_$(ASYN_VERS)),)
ASYN_COMPAT_$(ASYN_VERS)=$(ASYN_VERS)
endif

BUSSES += AsynDriver$(ASYN_COMPAT_$(ASYN_VERS))
endif

ifeq ($(LOADABLE_MODULE),YES)
SRCS += $(LIBRARY_DEFAULT)_registerRecordDeviceDriver.cpp
endif 
SRCS += $(STREAM_SRCS)
SRCS += $(RECORDS:%=dev%Stream.c)
SRCS += $(BUSSES:%=%Interface.cc)
SRCS += $(FORMATS:%=%Converter.c)

LIB_LIBS += Com dbIoc dbStaticIoc registryIoc iocsh

# memguard looks for memory leaks (gcc only)
# CPPFLAGS += -include ../memguard.h
# LIB_SRCS += memguard.cc

INC += devStream.h
INC += StreamBuffer.h
INC += StreamBusInterface.h
INC += StreamCore.h
INC += StreamError.h
INC += StreamFormatConverter.h
INC += StreamFormat.h
INC += StreamProtocol.h

include $(TOP)/configure/RULES

# Update version string (contains __DATE__ and __TIME__)
# each time make runs.
StreamVersion$(OBJ): FORCE
FORCE:

# create stream.dbd from all RECORDS
$(COMMON_DIR)/$(LIBRARY_DEFAULT).dbd:
	@for r in $(RECORDS); \
	do echo "device($$r,INST_IO,dev$${r}Stream,\"stream\")"; \
	done > $@
	@echo "driver(stream)" >> $@
	@echo "variable(streamDebug, int)" >> $@
	@echo "registrar(streamRegistrar)" >> $@
