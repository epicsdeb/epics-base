/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 * @date 2021.04.07
 */
#include <iocsh.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>
#include <pv/rpcService.h>

// The following must be the last include for code exampleLink uses
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvDatabase.h"
#include "pv/pvdbcrAddRecord.h"
using namespace epics::pvData;
using namespace std;

namespace epics { namespace pvDatabase {

PvdbcrAddRecordPtr PvdbcrAddRecord::create(
    std::string const & recordName,
    int asLevel,std::string const & asGroup)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr  topStructure = fieldCreate->createFieldBuilder()->
        addNestedStructure("argument")->
            add("recordName",pvString)->
            addNestedUnion("union") ->
                endNested()->
            endNested()->
        addNestedStructure("result") ->
            add("status",pvString) ->
            endNested()->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    PvdbcrAddRecordPtr pvRecord(
        new PvdbcrAddRecord(recordName,pvStructure,asLevel,asGroup));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

PvdbcrAddRecord::PvdbcrAddRecord(
    std::string const & recordName,
    PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup)
: PVRecord(recordName,pvStructure,asLevel,asGroup)
{
}

bool PvdbcrAddRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    pvRecordName = pvStructure->getSubField<PVString>("argument.recordName");
    if(!pvRecordName) return false;
    pvResult = pvStructure->getSubField<PVString>("result.status");
    if(!pvResult) return false;
    return true;
}

void PvdbcrAddRecord::process()
{
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    string name = pvRecordName->get();
    PVRecordPtr pvRecord = PVDatabase::getMaster()->findRecord(name);
    if(pvRecord) {
        pvResult->put(name + " already exists");
        return;
    }
    PVUnionPtr pvUnion = getPVStructure()->getSubField<PVUnion>("argument.union");
    if(!pvUnion) {
        pvResult->put(name + " argument.union is NULL");
        return;
    }
    PVFieldPtr pvField(pvUnion->get());
    if(!pvField) {
        pvResult->put(name + " union has no value");
        return;
    }
    if(pvField->getField()->getType()!=epics::pvData::structure) {
        pvResult->put(name + " union most be a structure");
        return;
    }
    StructureConstPtr st = std::tr1::static_pointer_cast<const Structure>(pvField->getField());
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(st);
    PVRecordPtr pvRec = PVRecord::create(name,pvStructure);
    bool result = PVDatabase::getMaster()->addRecord(pvRec);
    if(result) {
        pvResult->put("success");
    } else {
        pvResult->put("failure");
    }
}
}}

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg arg1 = { "asLevel", iocshArgInt };
static const iocshArg arg2 = { "asGroup", iocshArgString };
static const iocshArg *args[] = {&arg0,&arg1,&arg2};

static const iocshFuncDef pvdbcrAddRecordFuncDef = {"pvdbcrAddRecord", 3,args};

static void pvdbcrAddRecordCallFunc(const iocshArgBuf *args)
{
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrAddRecord recordName not specified");
    }
    string recordName = string(sval);
    int asLevel = args[1].ival;
    string asGroup("DEFAULT");
    sval = args[2].sval;
    if(sval) {
        asGroup = string(sval);
    }
    epics::pvDatabase::PvdbcrAddRecordPtr record = epics::pvDatabase::PvdbcrAddRecord::create(recordName);
    record->setAsLevel(asLevel);
    record->setAsGroup(asGroup);
    epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void pvdbcrAddRecord(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdbcrAddRecordFuncDef, pvdbcrAddRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(pvdbcrAddRecord);
}
