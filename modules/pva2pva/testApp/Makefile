TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CPPFLAGS += -I$(TOP)/pdbApp
USR_CPPFLAGS += -I$(TOP)/p2pApp
USR_CPPFLAGS += -I$(TOP)/common

TARGETS += $(COMMON_DIR)/p2pTestIoc.dbd
p2pTestIoc_DBD += base.dbd
# Tests explicitly create/destory PDB provider
#p2pTestIoc_DBD += qsrv.dbd

TARGETS += $(COMMON_DIR)/pvaLinkTestIoc.dbd
pvaLinkTestIoc_DBD += base.dbd
pvaLinkTestIoc_DBD += qsrv.dbd

PROD_SRCS += utilitiesx.cpp
PROD_LIBS += pvAccess pvData
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

TESTPROD_HOST += testweak
testweak_SRCS += testweak.cpp
testweak_LIBS += Com
TESTS += testweak

TESTPROD_HOST += testtest
testtest_SRCS += testtest.cpp
TESTS += testtest


TESTPROD_HOST += testpvif
testpvif_SRCS += testpvif.cpp
testpvif_SRCS += p2pTestIoc_registerRecordDeviceDriver.cpp
testpvif_LIBS += qsrv
TESTS += testpvif

TESTPROD_HOST += testpdb
testpdb_SRCS += testpdb.cpp
testpdb_SRCS += p2pTestIoc_registerRecordDeviceDriver.cpp
testpdb_LIBS += qsrv
TESTS += testpdb

TESTPROD_HOST += check_consist
check_consist_SRCS += check_consist.cpp

ifdef BASE_3_16
TESTPROD_HOST += testpvalink
testpvalink_SRCS += testpvalink.cpp
testpvalink_SRCS += pvaLinkTestIoc_registerRecordDeviceDriver.cpp
testpvalink_LIBS += qsrv
TESTS += testpvalink

TESTPROD_HOST += testgroupconfig
testgroupconfig_SRCS += testgroupconfig
testgroupconfig_LIBS += qsrv pvAccess pvData
testgroupconfig_LIBS += $(EPICS_BASE_IOC_LIBS)
TESTS += testgroupconfig

TESTPROD_HOST += testdbf_copy
testdbf_copy_SRCS += testdbf_copy
testdbf_copy_LIBS += qsrv pvAccess pvData
testdbf_copy_LIBS += $(EPICS_BASE_IOC_LIBS)
TESTS += testdbf_copy
endif

TESTSCRIPTS_HOST += $(TESTS:%=%.t)


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

