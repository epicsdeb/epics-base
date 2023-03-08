/* pvControl.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <string>
#include <stdexcept>

#define epicsExportSharedSymbols
#include <pv/pvType.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvControl.h>

namespace epics { namespace pvData {

using std::tr1::static_pointer_cast;
using std::string;

string PVControl::noControlFound("No control structure found");
string PVControl::notAttached("Not attached to an control structure");

bool PVControl::attach(PVFieldPtr const & pvField)
{
    if(pvField->getField()->getType()!=structure) return false;
    PVStructurePtr pvStructure = static_pointer_cast<PVStructure>(pvField);
    pvLow = pvStructure->getSubField<PVDouble>("limitLow");
    if(pvLow.get()==NULL) return false;
    pvHigh = pvStructure->getSubField<PVDouble>("limitHigh");
    if(pvHigh.get()==NULL) {
        pvLow.reset();
        return false;
    }
    pvMinStep = pvStructure->getSubField<PVDouble>("minStep");
    if(pvMinStep.get()==NULL) {
        pvLow.reset();
        pvHigh.reset();
        return false;
    }
    return true;
}

void PVControl::detach()
{
    pvLow.reset();
    pvHigh.reset();
}

bool PVControl::isAttached(){
    if(pvLow.get()==NULL) return false;
    return true;
}

void PVControl::get(Control &control) const
{
    if(pvLow.get()==NULL) {
        throw std::logic_error(notAttached);
    }
    control.setLow(pvLow->get());
    control.setHigh(pvHigh->get());
    control.setMinStep(pvMinStep->get());
}

bool PVControl::set(Control const & control)
{
    if(pvLow.get()==NULL) {
        throw std::logic_error(notAttached);
    }
    if(pvLow->isImmutable() || pvHigh->isImmutable() || pvMinStep->isImmutable()) return false;
    Control current;
    get(current);
    bool returnValue = false;
    if(current.getLow()!=control.getLow())
    {
         pvLow->put(control.getLow());
         returnValue = true;
    }
    if(current.getHigh()!=control.getHigh())
    {
         pvHigh->put(control.getHigh());
         returnValue = true;
    }
    if(current.getMinStep()!=control.getMinStep())
    {
         pvMinStep->put(control.getMinStep());
         returnValue = true;
    }
    return returnValue;
}

}}
