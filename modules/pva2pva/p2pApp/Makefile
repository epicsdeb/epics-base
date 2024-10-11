TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CPPFLAGS += -I$(TOP)/common

PROD_HOST = p2p

p2p_SRCS += gwmain.cpp

PROD_SRCS += server.cpp
PROD_SRCS += chancache.cpp
PROD_SRCS += moncache.cpp
PROD_SRCS += channel.cpp

PROD_LIBS += pvAccessIOC pvAccess pvData Com

TESTPROD_HOST += testmon
testmon_SRCS += testmon.cpp
testmon_SRCS += utilitiesx.cpp
TESTS += testmon

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

