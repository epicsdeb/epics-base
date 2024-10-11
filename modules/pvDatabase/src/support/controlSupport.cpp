/* controlSupport.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */

#include <pv/pvData.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include <pv/pvSupport.h>
#include "pv/pvDatabase.h"
#include "pv/controlSupport.h"

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase {

ControlSupport::~ControlSupport()
{
//cout << "ControlSupport::~ControlSupport()\n";
}

epics::pvData::StructureConstPtr ControlSupport::controlField(ScalarType scalarType)
{
    return FieldBuilder::begin()
            ->setId("control_t")
            ->add("limitLow", pvDouble)
            ->add("limitHigh", pvDouble)
            ->add("minStep", pvDouble)
            ->add("outputValue", scalarType)
            ->createStructure();
}


ControlSupportPtr ControlSupport::create(PVRecordPtr const & pvRecord)
{
   cerr << "ControlSupport IS DEPRECATED\n";
   ControlSupportPtr support(new ControlSupport(pvRecord));
   return support;
}

ControlSupport::ControlSupport(PVRecordPtr const & pvRecord)
   : pvRecord(pvRecord)
{}

bool ControlSupport::init(PVFieldPtr const & pv,PVFieldPtr const & pvsup)
{
    if(pv) {
         if(pv->getField()->getType()==epics::pvData::scalar) {
              ScalarConstPtr s = static_pointer_cast<const Scalar>(pv->getField());
              if(ScalarTypeFunc::isNumeric(s->getScalarType())) {
                   pvValue = static_pointer_cast<PVScalar>(pv);
              }
         }
    }
    if(!pvValue) {
        cout << "ControlSupport for record " << pvRecord->getRecordName()
        << " failed because not numeric scalar\n";
        return false;
    }
    pvControl = static_pointer_cast<PVStructure>(pvsup);
    if(pvControl) {
       pvLimitLow = pvControl->getSubField<PVDouble>("limitLow");
       pvLimitHigh = pvControl->getSubField<PVDouble>("limitHigh");
       pvMinStep = pvControl->getSubField<PVDouble>("minStep");
       pvOutputValue = pvControl->getSubField<PVScalar>("outputValue");
    }
    if(!pvControl || !pvLimitLow || !pvLimitHigh || !pvMinStep || !pvOutputValue) {
        cout << "ControlSupport for record " << pvRecord->getRecordName()
        << " failed because pvSupport not a valid control structure\n";
        return false;
    }
    ConvertPtr convert = getConvert();
    currentValue = convert->toDouble(pvValue);
    isMinStep = false;
    return true;
}

bool ControlSupport::process()
{
    ConvertPtr convert = getConvert();
    double value = convert->toDouble(pvValue);
    if(!isMinStep && value==currentValue) return false;
    double limitLow = pvLimitLow->get();
    double limitHigh = pvLimitHigh->get();
    double minStep = pvMinStep->get();
    bool setValue = false;
    if(limitHigh>limitLow) {
        if(value>limitHigh) {value = limitHigh;setValue=true;}
        if(value<limitLow) {value = limitLow;setValue=true;}
    }
    if(setValue) convert->fromDouble(pvValue,value);
    double diff = value - currentValue;
    double outputValue = value;
    if(minStep>0.0) {
        if(diff<0.0) {
            outputValue = currentValue - minStep;
            if(limitHigh>limitLow && outputValue<=limitLow) outputValue = limitLow;
            isMinStep = true;
            if(outputValue<value) {
                 outputValue = value;
                 isMinStep = false;
            }
        } else {
            outputValue = currentValue + minStep;
            if(limitHigh>limitLow && outputValue>=limitHigh) outputValue = limitHigh;
            isMinStep = true;
            if(outputValue>value)  {
                 outputValue = value;
                 isMinStep = false;
            }
        }
    }
    if(outputValue==currentValue) return false;
    currentValue = outputValue;
    convert->fromDouble(pvOutputValue,outputValue);
    return true;
}

void ControlSupport::reset()
{
   isMinStep = false;
}


}}
