TOP=..

include $(TOP)/configure/CONFIG

PROD_LIBS += nt pvData Com

TESTPROD_HOST += ntfieldTest
ntfieldTest_SRCS += ntfieldTest.cpp
TESTS += ntfieldTest

TESTPROD_HOST += ntscalarTest
ntscalarTest_SRCS += ntscalarTest.cpp
TESTS += ntscalarTest

TESTPROD_HOST += ntscalarArrayTest
ntscalarArrayTest_SRCS += ntscalarArrayTest.cpp
TESTS += ntscalarArrayTest

TESTPROD_HOST += ntnameValueTest
ntnameValueTest_SRCS += ntnameValueTest.cpp
TESTS += ntnameValueTest

TESTPROD_HOST += ntmultiChannelTest
ntmultiChannelTest_SRCS += ntmultiChannelTest.cpp
TESTS += ntmultiChannelTest

TESTPROD_HOST += ntscalarMultiChannelTest
ntscalarMultiChannelTest_SRCS += ntscalarMultiChannelTest.cpp
TESTS += ntscalarMultiChannelTest

TESTPROD_HOST += nttableTest
nttableTest_SRCS = nttableTest.cpp
TESTS += nttableTest

TESTPROD_HOST += ntndarrayTest
ntndarrayTest_SRCS = ntndarrayTest.cpp
TESTS += ntndarrayTest

TESTPROD_HOST += ntmatrixTest
ntmatrixTest_SRCS = ntmatrixTest.cpp
TESTS += ntmatrixTest

TESTPROD_HOST += ntenumTest
ntenumTest_SRCS = ntenumTest.cpp
TESTS += ntenumTest

TESTPROD_HOST += ntunionTest
ntunionTest_SRCS = ntunionTest.cpp
TESTS += ntunionTest

TESTPROD_HOST += ntaggregateTest
ntaggregateTest_SRCS = ntaggregateTest.cpp
TESTS += ntaggregateTest

TESTPROD_HOST += ntattributeTest
ntattributeTest_SRCS = ntattributeTest.cpp
TESTS += ntattributeTest

TESTPROD_HOST += ntndarrayAttributeTest
ntndarrayAttributeTest_SRCS = ntndarrayAttributeTest.cpp
TESTS += ntndarrayAttributeTest

TESTPROD_HOST += ntcontinuumTest
ntcontinuumTest = ntcontinuumTest.cpp
TESTS += ntcontinuumTest

TESTPROD_HOST += nthistogramTest
nthistogramTest_SRCS = nthistogramTest.cpp
TESTS += nthistogramTest

TESTPROD_HOST += ntutilsTest
ntutilsTest_SRCS = ntutilsTest.cpp
TESTS += ntutilsTest

TESTPROD_HOST += validatorTest
validatorTest_SRCS = validatorTest.cpp
TESTS += validatorTest

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

