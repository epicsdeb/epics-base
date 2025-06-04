/* scalarAlarmSupport.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>

#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/pvSupport.h"
#include "pv/pvDatabase.h"
#include "pv/scalarAlarmSupport.h"

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase {

ScalarAlarmSupport::~ScalarAlarmSupport()
{
//cout << "ScalarAlarmSupport::~ScalarAlarmSupport()\n";
}


epics::pvData::StructureConstPtr ScalarAlarmSupport::scalarAlarmField()
{
    return FieldBuilder::begin()
            ->setId("scalarAlarm_t")
            ->add("lowAlarmLimit", pvDouble)
            ->add("lowWarningLimit", pvDouble)
            ->add("highWarningLimit", pvDouble)
            ->add("highAlarmLimit", pvDouble)
            ->add("hysteresis", pvDouble)
            ->createStructure();
}

ScalarAlarmSupportPtr ScalarAlarmSupport::create(PVRecordPtr const & pvRecord)
{
   cerr << "ScalarAlarmSupport IS DEPRECATED\n";
   ScalarAlarmSupportPtr support(new ScalarAlarmSupport(pvRecord));
   return support;
}

ScalarAlarmSupport::ScalarAlarmSupport(PVRecordPtr const & pvRecord)
   : pvRecord(pvRecord),
     prevAlarmRange(range_Undefined)
{}


bool ScalarAlarmSupport::init(
    PVFieldPtr const & pvval,
    PVStructurePtr const & pvalarm,
    PVFieldPtr const & pvsup)
{
    if(pvval->getField()->getType()==epics::pvData::scalar) {
        ScalarConstPtr s = static_pointer_cast<const Scalar>(pvval->getField());
        if(ScalarTypeFunc::isNumeric(s->getScalarType())) {
            pvValue = static_pointer_cast<PVScalar>(pvval);
        }
    }
    if(!pvValue) {
        cout << "ScalarAlarmSupport for record " << pvRecord->getRecordName()
        << " failed because not numeric scalar\n";
        return false;
    }
    pvScalarAlarm = static_pointer_cast<PVStructure>(pvsup);
    if(pvScalarAlarm) {
       pvLowAlarmLimit = pvScalarAlarm->getSubField<PVDouble>("lowAlarmLimit");
       pvLowWarningLimit = pvScalarAlarm->getSubField<PVDouble>("lowWarningLimit");
       pvHighWarningLimit = pvScalarAlarm->getSubField<PVDouble>("highWarningLimit");
       pvHighAlarmLimit = pvScalarAlarm->getSubField<PVDouble>("highAlarmLimit");
       pvHysteresis = pvScalarAlarm->getSubField<PVDouble>("hysteresis");
    }
    if(!pvScalarAlarm
       || !pvLowAlarmLimit || !pvLowWarningLimit
       || !pvLowWarningLimit || !pvHighAlarmLimit
       || !pvHysteresis)
    {
        cout << "ScalarAlarmSupport for record " << pvRecord->getRecordName()
        << " failed because pvSupport not a valid scalarAlarm structure\n";
        return false;
    }
    pvAlarm = pvalarm;
    ConvertPtr convert = getConvert();
    requestedValue = convert->toDouble(pvValue);
    currentValue = requestedValue;
    isHystersis = false;
    setAlarm(pvAlarm,range_Undefined);
    return true;
}

bool ScalarAlarmSupport::process()
{
    ConvertPtr convert = getConvert();
    double value = convert->toDouble(pvValue);
    double lowAlarmLimit = pvLowAlarmLimit->get();
    double lowWarningLimit = pvLowWarningLimit->get();
    double highWarningLimit = pvHighWarningLimit->get();
    double highAlarmLimit = pvHighAlarmLimit->get();
    double hysteresis = pvHysteresis->get();
    int alarmRange = range_Normal;
    if(highAlarmLimit>lowAlarmLimit) {
         if(value>=highAlarmLimit
         ||(prevAlarmRange==range_Hihi && value>=highAlarmLimit-hysteresis)) {
             alarmRange = range_Hihi;
         } else if(value<=lowAlarmLimit
         ||(prevAlarmRange==range_Lolo && value<=lowAlarmLimit+hysteresis)) {
             alarmRange = range_Lolo;
         }
    }
    if(alarmRange==range_Normal && (highWarningLimit>lowWarningLimit)) {
         if(value>=highWarningLimit
         ||(prevAlarmRange==range_High && value>=highWarningLimit-hysteresis)) {
             alarmRange = range_High;
         } else if(value<=lowWarningLimit
         ||(prevAlarmRange==range_Low && value<=lowWarningLimit+hysteresis)) {
             alarmRange = range_Low;
         }
    }
    bool retValue = false;
    if(alarmRange!=prevAlarmRange) {
        setAlarm(pvAlarm,alarmRange);
        retValue = true;
    }
    prevAlarmRange = alarmRange;
    currentValue = value;
    return retValue;
}

void ScalarAlarmSupport::reset()
{
   isHystersis = false;
}

void ScalarAlarmSupport::setAlarm(
        epics::pvData::PVStructurePtr const & pva,
        int alarmRange)
{
    Alarm alarm;
    PVAlarm pvAlarm;
    if(!pvAlarm.attach(pva)) throw std::logic_error("bad alarm field");
    epics::pvData::AlarmStatus status(epics::pvData::noStatus);
    epics::pvData::AlarmSeverity severity(epics::pvData::noAlarm);
    string message;
    switch (alarmRange) {
    case range_Lolo :
        {
            severity = epics::pvData::majorAlarm;
            status = epics::pvData::recordStatus;
            message = "major low alarm";
            break;
        }
    case range_Low :
        {
            severity = epics::pvData::minorAlarm;
            status = epics::pvData::recordStatus;
            message = "minor low alarm";
            break;
        }
    case range_Normal :
        {
            break;
        }
    case range_High :
        {
            severity = epics::pvData::minorAlarm;
            status = epics::pvData::recordStatus;
            message = "minor high alarm";
            break;
        }
    case range_Hihi :
        {
            severity = epics::pvData::majorAlarm;
            status = epics::pvData::recordStatus;
            message = "major high alarm";
            break;
        }
    case range_Invalid :
        {
            severity = epics::pvData::invalidAlarm;
            status = epics::pvData::recordStatus;
            message = "invalid alarm";
            break;
        }
    case range_Undefined :
        {
            severity = epics::pvData::undefinedAlarm;
            status = epics::pvData::recordStatus;
            message = "undefined alarm";
            break;
        }
    default:
        {
            severity = epics::pvData::undefinedAlarm;
            status = epics::pvData::recordStatus;
            message = "bad alarm definition";
            break;
        }
    }
    alarm.setStatus(status);
    alarm.setSeverity(severity);
    alarm.setMessage(message);
    pvAlarm.set(alarm);
}


}}
