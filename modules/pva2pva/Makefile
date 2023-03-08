# Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

# Directories to build, any order
DIRS += configure
DIRS += $(wildcard *App)
DIRS += $(wildcard iocBoot)

# pdbApp depends on configure for CONFIG_QSRV_VERSION
pdbApp_DEPEND_DIRS = configure

# iocBoot depends on all *App dirs
iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))
testApp_DEPEND_DIRS += p2pApp pdbApp

# Add any additional dependency rules here:

include $(TOP)/configure/RULES_TOP
