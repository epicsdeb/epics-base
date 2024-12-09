#*************************************************************************
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution.
#*************************************************************************
TOP=..

include $(TOP)/configure/CONFIG

CFG += CONFIG_PVACCESS_VERSION
CFG += CONFIG_PVACCESS_MODULE

TARGETS = $(CONFIG_TARGETS)
CONFIGS += $(subst ../,,$(wildcard $(CONFIG_INSTALLS)))

include $(TOP)/configure/RULES
