# Makefile for the pvDatabase library

TOP = ..
include $(TOP)/configure/CONFIG

PVDATABASE_SRC = $(TOP)/src


INC += pv/pvPlugin.h
INC += pv/pvStructureCopy.h
INC += pv/pvArrayPlugin.h
INC += pv/pvDeadbandPlugin.h
INC += pv/pvTimestampPlugin.h

INC += pv/pvDatabase.h

INC += pv/channelProviderLocal.h

INC += pv/pvSupport.h
INC += pv/controlSupport.h
INC += pv/scalarAlarmSupport.h

INC += pv/pvdbcrScalarRecord.h
INC += pv/pvdbcrScalarArrayRecord.h
INC += pv/pvdbcrAddRecord.h
INC += pv/pvdbcrRemoveRecord.h
INC += pv/pvdbcrProcessRecord.h
INC += pv/pvdbcrTraceRecord.h

include $(PVDATABASE_SRC)/copy/Makefile
include $(PVDATABASE_SRC)/database/Makefile
include $(PVDATABASE_SRC)/pvAccess/Makefile
include $(PVDATABASE_SRC)/support/Makefile
include $(PVDATABASE_SRC)/special/Makefile

LIBRARY += pvDatabase
pvDatabase_LIBS += pvData
pvDatabase_LIBS += pvAccess
LIB_LIBS += Com

# shared library ABI version.
SHRLIB_VERSION ?= $(EPICS_PVDATABASE_MAJOR_VERSION).$(EPICS_PVDATABASE_MINOR_VERSION).$(EPICS_PVDATABASE_MAINTENANCE_VERSION)

# needed for Windows
LIB_SYS_LIBS_WIN32 += netapi32 ws2_32

include $(TOP)/configure/RULES
