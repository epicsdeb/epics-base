#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS += configure
DIRS += src
src_DEPEND_DIRS = configure
DIRS += test
test_DEPEND_DIRS = src

include $(TOP)/configure/RULES_TOP


