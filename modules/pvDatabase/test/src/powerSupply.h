/* powerSupply.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */
#ifndef POWERSUPPLY_H
#define POWERSUPPLY_H


#ifdef epicsExportSharedSymbols
#   define powerSupplyEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/timeStamp.h>
#include <pv/alarm.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvDatabase.h>

#ifdef powerSupplyEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#       undef powerSupplyEpicsExportSharedSymbols
#endif

#include <shareLib.h>

//epicsShareFunc epics::pvData::PVStructurePtr createPowerSupply();

namespace epics { namespace pvDatabase {

class PowerSupply;
typedef std::tr1::shared_ptr<PowerSupply> PowerSupplyPtr;

class PowerSupply :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(PowerSupply);
    static PowerSupplyPtr create(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    virtual ~PowerSupply();
    virtual bool init();
    virtual void process();
    void put(double power,double voltage);
    double getPower();
    double getVoltage();
    double getCurrent();
private:
    PowerSupply(std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVDoublePtr pvCurrent;
    epics::pvData::PVDoublePtr pvPower;
    epics::pvData::PVDoublePtr pvVoltage;
    epics::pvData::PVAlarm pvAlarm;
    epics::pvData::PVTimeStamp pvTimeStamp;
    epics::pvData::Alarm alarm;
    epics::pvData::TimeStamp timeStamp;
};

inline epics::pvData::PVStructurePtr createPowerSupply()
{
    epics::pvData::FieldCreatePtr fieldCreate = epics::pvData::getFieldCreate();
    epics::pvData::StandardFieldPtr standardField = epics::pvData::getStandardField();
    epics::pvData::PVDataCreatePtr pvDataCreate = epics::pvData::getPVDataCreate();

    return pvDataCreate->createPVStructure(
        fieldCreate->createFieldBuilder()->
            add("alarm",standardField->alarm()) ->
            add("timeStamp",standardField->timeStamp()) ->
            addNestedStructure("power") ->
               add("value",epics::pvData::pvDouble) ->
               add("alarm",standardField->alarm()) ->
               endNested()->
            addNestedStructure("voltage") ->
               add("value",epics::pvData::pvDouble) ->
               add("alarm",standardField->alarm()) ->
               endNested()->
            addNestedStructure("current") ->
               add("value",epics::pvData::pvDouble) ->
               add("alarm",standardField->alarm()) ->
               endNested()->
            createStructure());
}

inline PowerSupplyPtr PowerSupply::create(
    std::string const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
{
    PowerSupplyPtr pvRecord(
        new PowerSupply(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

inline PowerSupply::PowerSupply(
    std::string const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

inline PowerSupply::~PowerSupply()
{
}

inline bool PowerSupply::init()
{
    initPVRecord();
    epics::pvData::PVStructurePtr pvStructure = getPVStructure();
    epics::pvData::PVFieldPtr pvField;
    bool result;
    pvField = pvStructure->getSubField("timeStamp");
    if(!pvField) {
        std::cerr << "no timeStamp" << std::endl;
        return false;
    }
    result = pvTimeStamp.attach(pvField);
    if(!result) {
        std::cerr << "no timeStamp" << std::endl;
        return false;
    }
    pvField = pvStructure->getSubField("alarm");
    if(!pvField) {
        std::cerr << "no alarm" << std::endl;
        return false;
    }
    result = pvAlarm.attach(pvField);
    if(!result) {
        std::cerr << "no alarm" << std::endl;
        return false;
    }
    pvCurrent = pvStructure->getSubField<epics::pvData::PVDouble>("current.value");
    if(!pvCurrent) {
        std::cerr << "no current\n";
        return false;
    }
    pvVoltage = pvStructure->getSubField<epics::pvData::PVDouble>("voltage.value");
    if(!pvVoltage) {
        std::cerr << "no current\n";
        return false;
    }
    pvPower = pvStructure->getSubField<epics::pvData::PVDouble>("power.value");
    if(!pvPower) {
        std::cerr << "no powert\n";
        return false;
    }
    return true;
}

inline void PowerSupply::process()
{
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
    double voltage = pvVoltage->get();
    double power = pvPower->get();
    if(voltage<1e-3 && voltage>-1e-3) {
        alarm.setMessage("bad voltage");
        alarm.setSeverity(epics::pvData::majorAlarm);
        pvAlarm.set(alarm);
        return;
    }
    double current = power/voltage;
    pvCurrent->put(current);
    alarm.setMessage("");
    alarm.setSeverity(epics::pvData::noAlarm);
    pvAlarm.set(alarm);
}

inline void PowerSupply::put(double power,double voltage)
{
    pvPower->put(power);
    pvVoltage->put(voltage);
}

inline double PowerSupply::getPower()
{
    return pvPower->get();
}

inline double PowerSupply::getVoltage()
{
    return pvVoltage->get();
}

inline double PowerSupply::getCurrent()
{
    return pvCurrent->get();
}


}}

#endif  /* POWERSUPPLY_H */
