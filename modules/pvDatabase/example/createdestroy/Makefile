TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the application

TESTPROD_HOST = createdestroy

createdestroy_SRCS += createdestroy.cpp

# Add all the support libraries needed by this application
#pvatest_LIBS += xxx

# Finally link to the EPICS Base libraries
createdestroy_LIBS += pvDatabase pvAccess pvData
createdestroy_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
