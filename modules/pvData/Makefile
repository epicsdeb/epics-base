# Makefile for the EPICS V4 pvData module

TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure

DIRS += src
src_DEPEND_DIRS = configure

DIRS += testApp
testApp_DEPEND_DIRS = src

DIRS += examples
examples_DEPEND_DIRS = src

include $(TOP)/configure/RULES_TOP
