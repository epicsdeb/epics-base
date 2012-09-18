#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE Versions 3.13.7
# and higher are distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#*************************************************************************
#
# Revision-Id: anj@aps.anl.gov-20101005192737-disfz3vs0f3fiixd
#

TOP = .
include $(TOP)/configure/CONFIG

# Bootstrap resolution: tools not installed yet
TOOLS = $(TOP)/src/tools

DIRS += configure src
ifeq ($(findstring YES,$(COMPAT_313) $(COMPAT_TOOLS_313)),YES)
DIRS += config
endif

src_DEPEND_DIRS = configure
config_DEPEND_DIRS = src

include $(TOP)/configure/RULES_TOP

