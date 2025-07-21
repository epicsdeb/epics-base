# Makefile for the pvData tests

TOP = ..
include $(TOP)/configure/CONFIG

PVDATABASE_TEST = $(TOP)/test

PROD_LIBS += pvDatabase
PROD_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(PVDATABASE_TEST)/src/Makefile

# pvDatabaseAllTests runs all the test programs in a known working order.
testHarness_SRCS += pvDatabaseAllTests.c

# Name the application pvDbTestHarness
pvDbTestHarness_SRCS = $(testHarness_SRCS)

# Build for vxWorks
PROD_vxWorks = pvDbTestHarness
TESTSPEC_vxWorks = pvDbTestHarness.$(MUNCH_SUFFIX); pvDatabaseAllTests

# Build for RTEMS, with harness code & configuration
PROD_RTEMS += pvDbTestHarness
pvDbTestHarness_SRCS_RTEMS += rtemsTestHarness.c
TESTSPEC_RTEMS = pvDbTestHarness.$(MUNCH_SUFFIX); pvDatabaseAllTests

# Build test scripts for hosts
TESTSCRIPTS_HOST += $(TESTS:%=%.t)

include $(TOP)/configure/RULES
