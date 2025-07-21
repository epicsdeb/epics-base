# This is a Makefile fragment, see ../Makefile

TOP = ..
include $(TOP)/configure/CONFIG

LIBRARY += pvaClient

# shared library ABI version.
SHRLIB_VERSION ?= $(EPICS_PVACLIENT_MAJOR_VERSION).$(EPICS_PVACLIENT_MINOR_VERSION).$(EPICS_PVACLIENT_MAINTENANCE_VERSION)

INC += pv/pvaClient.h
INC += pv/pvaClientMultiChannel.h

LIBSRCS += pvaClient.cpp
LIBSRCS += pvaClientData.cpp
LIBSRCS += pvaClientPutData.cpp
LIBSRCS += pvaClientGetData.cpp
LIBSRCS += pvaClientMonitorData.cpp
LIBSRCS += pvaClientChannel.cpp
LIBSRCS += pvaClientProcess.cpp
LIBSRCS += pvaClientGet.cpp
LIBSRCS += pvaClientPut.cpp
LIBSRCS += pvaClientMonitor.cpp
LIBSRCS += pvaClientPutGet.cpp
LIBSRCS += pvaClientMultiChannel.cpp
LIBSRCS += pvaClientMultiGetDouble.cpp
LIBSRCS += pvaClientMultiPutDouble.cpp
LIBSRCS += pvaClientMultiMonitorDouble.cpp
LIBSRCS += pvaClientNTMultiPut.cpp
LIBSRCS += pvaClientNTMultiData.cpp
LIBSRCS += pvaClientNTMultiGet.cpp
LIBSRCS += pvaClientNTMultiMonitor.cpp
LIBSRCS += pvaClientRPC.cpp

pvaClient_LIBS += nt
pvaClient_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
pvaClient_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
