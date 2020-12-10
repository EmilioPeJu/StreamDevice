TOP=..
include $(TOP)/configure/CONFIG
LIBRARY = pcre

USR_CFLAGS += -DHAVE_CONFIG_H
LIB_SRCS += pcre_chartables.c
LIB_SRCS += pcre_compile.c
LIB_SRCS += pcre_config.c
LIB_SRCS += pcre_dfa_exec.c
LIB_SRCS += pcre_exec.c
LIB_SRCS += pcre_fullinfo.c
LIB_SRCS += pcre_get.c
LIB_SRCS += pcre_globals.c
LIB_SRCS += pcre_info.c
LIB_SRCS += pcre_maketables.c
LIB_SRCS += pcre_newline.c
LIB_SRCS += pcre_ord2utf8.c
LIB_SRCS += pcre_refcount.c
LIB_SRCS += pcre_study.c
LIB_SRCS += pcre_tables.c
LIB_SRCS += pcre_try_flipped.c
LIB_SRCS += pcre_ucp_searchfuncs.c
LIB_SRCS += pcre_valid_utf8.c
LIB_SRCS += pcre_version.c
LIB_SRCS += pcre_xclass.c

INC += pcre.h

include $(TOP)/configure/RULES
