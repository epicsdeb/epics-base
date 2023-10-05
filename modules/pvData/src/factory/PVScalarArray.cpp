/*PVScalarArray.cpp*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>

#define epicsExportSharedSymbols
#include <pv/pvData.h>
#include <pv/factory.h>

using std::tr1::static_pointer_cast;using std::tr1::static_pointer_cast;

namespace epics { namespace pvData {

    PVScalarArray::~PVScalarArray() {}

    PVScalarArray::PVScalarArray(ScalarArrayConstPtr const & scalarArray)
    : PVArray(scalarArray) {}

    const ScalarArrayConstPtr PVScalarArray::getScalarArray() const
    {
       return static_pointer_cast<const ScalarArray>(PVField::getField());
    }

}}
