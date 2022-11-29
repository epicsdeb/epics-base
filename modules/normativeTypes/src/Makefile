TOP = ..
include $(TOP)/configure/CONFIG

INC += pv/nt.h
INC += pv/ntutils.h
INC += pv/ntid.h
INC += pv/ntfield.h
INC += pv/ntscalar.h
INC += pv/ntscalarArray.h
INC += pv/ntnameValue.h
INC += pv/nttable.h
INC += pv/ntmultiChannel.h
INC += pv/ntscalarMultiChannel.h
INC += pv/ntndarray.h
INC += pv/ntmatrix.h
INC += pv/ntenum.h
INC += pv/ntunion.h
INC += pv/ntaggregate.h
INC += pv/ntattribute.h
INC += pv/ntcontinuum.h
INC += pv/nthistogram.h
INC += pv/nturi.h
INC += pv/ntndarrayAttribute.h

LIBSRCS += ntutils.cpp
LIBSRCS += ntid.cpp
LIBSRCS += ntfield.cpp
LIBSRCS += ntscalar.cpp
LIBSRCS += ntscalarArray.cpp
LIBSRCS += ntnameValue.cpp
LIBSRCS += nttable.cpp
LIBSRCS += ntmultiChannel.cpp
LIBSRCS += ntscalarMultiChannel.cpp
LIBSRCS += ntndarray.cpp
LIBSRCS += ntmatrix.cpp
LIBSRCS += ntenum.cpp
LIBSRCS += ntunion.cpp
LIBSRCS += ntaggregate.cpp
LIBSRCS += ntattribute.cpp
LIBSRCS += ntcontinuum.cpp
LIBSRCS += nthistogram.cpp
LIBSRCS += nturi.cpp
LIBSRCS += ntndarrayAttribute.cpp

LIBRARY = nt

nt_LIBS += pvData Com

# shared library ABI version.
SHRLIB_VERSION ?= $(EPICS_NTYPES_MAJOR_VERSION).$(EPICS_NTYPES_MINOR_VERSION).$(EPICS_NTYPES_MAINTENANCE_VERSION)

include $(TOP)/configure/RULES

