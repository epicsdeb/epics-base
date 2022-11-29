# Makefile for the pvData library

TOP = ..
include $(TOP)/configure/CONFIG

PVDATA_SRC = $(TOP)/src

include $(PVDATA_SRC)/misc/Makefile
include $(PVDATA_SRC)/pv/Makefile
include $(PVDATA_SRC)/factory/Makefile
include $(PVDATA_SRC)/property/Makefile
include $(PVDATA_SRC)/copy/Makefile
include $(PVDATA_SRC)/pvMisc/Makefile
include $(PVDATA_SRC)/json/Makefile

LIBRARY = pvData

pvData_LIBS += Com

EXPANDVARS += EPICS_PVD_MAJOR_VERSION
EXPANDVARS += EPICS_PVD_MINOR_VERSION
EXPANDVARS += EPICS_PVD_MAINTENANCE_VERSION
EXPANDVARS += EPICS_PVD_DEVELOPMENT_FLAG

EXPANDFLAGS += $(foreach var,$(EXPANDVARS),-D$(var)="$(strip $($(var)))")

# shared library ABI version.
SHRLIB_VERSION = $(EPICS_PVD_MAJOR_VERSION).$(EPICS_PVD_MINOR_VERSION).$(EPICS_PVD_MAINTENANCE_VERSION)

include $(TOP)/configure/RULES

# Can't use EXPAND as generated headers must appear
# in O.Common, but EXPAND emits rules for O.$(T_A)
../O.Common/pv/pvdVersionNum.h: ../pv/pvdVersionNum.h@
	$(MKDIR) $(COMMON_DIR)/pv
	$(EXPAND_TOOL) $(EXPANDFLAGS) $($@_EXPANDFLAGS) $< $@
