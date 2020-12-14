TOP = ..
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard simulations))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard makeIocs))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard makeDocumentation))
include $(TOP)/configure/RULES_DIRS
