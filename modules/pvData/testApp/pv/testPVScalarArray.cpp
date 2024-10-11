/* testPVScalarArray.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2010.11 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>

#include <epicsAssert.h>
#include <epicsExit.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::cout;
using std::endl;

namespace {

static void testFactory()
{
    testDiag("Check array creation");

    for(ScalarType e=pvBoolean; e<=pvString; e=(ScalarType)(1+(int)e))
    {
        testDiag("Check type %s", ScalarTypeFunc::name(e));
        PVScalarArrayPtr arr = getPVDataCreate()->createPVScalarArray(e);
        testOk1(arr.get()!=NULL);
        if(!arr.get())
            continue;
        testOk1(arr->getScalarArray()->getElementType()==e);
        testOk1(arr->getLength()==0);
        arr->setLength(10);
        testOk1(arr->getLength()==10);
        testOk1(arr->getCapacity()>=10);
        arr->setLength(0);
        testOk1(arr->getLength()==0);
    }
}

template<typename PVT>
bool hasUniqueVector(const typename PVT::shared_pointer& pv)
{
    typename PVT::const_svector data;
    pv->swap(data);
    bool ret = data.unique();
    pv->swap(data);
    return ret;
}

template<typename PVT>
struct basicTestData {
    static inline void fill(typename PVT::svector& data) {
        data.resize(100);
        for(size_t i=0; i<data.size(); i++)
        {
            data[i] = static_cast<typename PVT::svector::element_type>(10*i);
        }
    }
};


template<>
struct basicTestData<PVStringArray> {
    static inline void fill(PVStringArray::svector& data) {
        PVIntArray::svector idata;
        basicTestData<PVIntArray>::fill(idata);
        data.resize(idata.size());
        castUnsafeV(data.size(), pvString, data.data(), pvInt, idata.data());
    }
};

template<typename PVT>
static void testBasic()
{
    testDiag("Check basic array operations for %s", typeid(PVT).name());

    typename PVT::shared_pointer arr1 = static_pointer_cast<PVT>(getPVDataCreate()->createPVScalarArray(PVT::typeCode));
    typename PVT::shared_pointer arr2 = static_pointer_cast<PVT>(getPVDataCreate()->createPVScalarArray(PVT::typeCode));

    testOk1(*arr1==*arr2);
    testOk1(*arr1==*arr1);
    testOk1(*arr1->getScalarArray()==*arr2->getScalarArray());

    typename PVT::svector data;
    data.reserve(200);
    basicTestData<PVT>::fill(data);

    typename PVT::const_svector cdata(freeze(data));

    testOk1(cdata.unique());
    arr1->replace(cdata);
    testOk1(!cdata.unique());

    {
        typename PVT::const_svector avoid;
        arr1->PVScalarArray::template getAs<typename PVT::value_type>(avoid);
        testOk1(avoid.data()==cdata.data());
        testOk1(avoid.data()==arr1->view().data());
    }

    testOk1(arr1->getLength()==cdata.size());

    testOk1(*arr1!=*arr2);

    cdata.clear();

    testOk1(hasUniqueVector<PVT>(arr1));

    arr2->assign(*arr1);

    testOk1(*arr1==*arr2);
    testOk1(!hasUniqueVector<PVT>(arr1));

    arr2->swap(cdata);
    arr2->postPut();

    testOk1(arr2->getLength()==0);
    testOk1(cdata.size()==arr1->getLength());

    PVIntArray::const_svector idata;
    arr1->PVScalarArray::template getAs<int32>(idata);

    testOk1(idata.at(1)==10);

    PVIntArray::svector wdata(thaw(idata));

    wdata.at(1) = 42;

    idata = freeze(wdata);

    arr1->PVScalarArray::template putFrom<int32>(idata);

    testOk1(castUnsafe<PVIntArray::value_type>(arr1->view()[1])==42);
}

static void testShare()
{
    testDiag("Check array data sharing");

    PVIntArrayPtr iarr = static_pointer_cast<PVIntArray>(getPVDataCreate()->createPVScalarArray(pvInt));
    PVStringArrayPtr sarr = static_pointer_cast<PVStringArray>(getPVDataCreate()->createPVScalarArray(pvString));

    PVIntArray::const_svector idata(4, 1);

    sarr->PVScalarArray::putFrom<int32>(idata); // copy and convert

    testOk1(idata.unique());

    iarr->PVScalarArray::putFrom<int32>(idata); // take a reference

    testOk1(!idata.unique());

    idata.clear();
    PVIntArray::const_svector cdata;

    sarr->PVScalarArray::getAs<int32>(cdata); // copy and convert

    testOk1(cdata.unique());

    iarr->PVScalarArray::getAs<int32>(cdata); // take a reference

    testOk1(!cdata.unique());
}

static void testVoid()
{
    testDiag("Check PVScalarArray put/get from void");

    PVIntArrayPtr iarr = static_pointer_cast<PVIntArray>(getPVDataCreate()->createPVScalarArray(pvInt));

    PVIntArray::const_svector idata(4, 1);
    iarr->PVScalarArray::putFrom(idata);
    idata.clear();

    shared_vector<const void> cvbuf;
    iarr->PVScalarArray::getAs(cvbuf);

    idata = static_shared_vector_cast<const PVIntArray::value_type>(cvbuf);
    testOk1(idata.size()==4);

    iarr->PVScalarArray::putFrom(cvbuf);

    testOk1(iarr->getLength()==4);
}

} // end namespace

MAIN(testPVScalarArray)
{
    testPlan(158);
    testFactory();
    testBasic<PVByteArray>();
    testBasic<PVUByteArray>();
    testBasic<PVIntArray>();
    testBasic<PVDoubleArray>();
    testBasic<PVStringArray>();
    testShare();
    testVoid();
    return testDone();
}
