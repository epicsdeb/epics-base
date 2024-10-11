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

// The following must be the last include
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvDatabase.h"
#include "pv/pvdbcrTraceRecord.h"
using namespace epics::pvData;
using namespace std;

namespace epics { namespace pvDatabase {

PvdbcrTraceRecordPtr PvdbcrTraceRecord::create(
    std::string const & recordName,
    int asLevel,std::string const & asGroup)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr  topStructure = fieldCreate->createFieldBuilder()->
        addNestedStructure("argument")->
            add("recordName",pvString)->
            add("level",pvInt)->
            endNested()->
        addNestedStructure("result") ->
            add("status",pvString) ->
            endNested()->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    PvdbcrTraceRecordPtr pvRecord(
        new PvdbcrTraceRecord(recordName,pvStructure,asLevel,asGroup));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

PvdbcrTraceRecord::PvdbcrTraceRecord(
    std::string const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup)
: PVRecord(recordName,pvStructure,asLevel,asGroup)
{
}


bool PvdbcrTraceRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    pvRecordName = pvStructure->getSubField<PVString>("argument.recordName");
    if(!pvRecordName) return false;
    pvLevel = pvStructure->getSubField<PVInt>("argument.level");
    if(!pvLevel) return false;
    pvResult = pvStructure->getSubField<PVString>("result.status");
    if(!pvResult) return false;
    return true;
}

void PvdbcrTraceRecord::process()
{
    string name = pvRecordName->get();
    PVRecordPtr pvRecord = PVDatabase::getMaster()->findRecord(name);
    if(!pvRecord) {
        pvResult->put(name + " not found");
        return;
    }
    pvRecord->setTraceLevel(pvLevel->get());
    pvResult->put("success");
}
}}

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg arg1 = { "asLevel", iocshArgInt };
static const iocshArg arg2 = { "asGroup", iocshArgString };
static const iocshArg *args[] = {&arg0,&arg1,&arg2};

static const iocshFuncDef pvdbcrTraceRecordFuncDef = {"pvdbcrTraceRecord", 3,args};

static void pvdbcrTraceRecordCallFunc(const iocshArgBuf *args)
{
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrTraceRecord recordName not specified");
    }
    string recordName = string(sval);
    int asLevel = args[1].ival;
    string asGroup("DEFAULT");
    sval = args[2].sval;
    if(sval) {
        asGroup = string(sval);
    }
    epics::pvDatabase::PvdbcrTraceRecordPtr record
        = epics::pvDatabase::PvdbcrTraceRecord::create(recordName);
    record->setAsLevel(asLevel);
    record->setAsGroup(asGroup);
    epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void pvdbcrTraceRecord(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdbcrTraceRecordFuncDef, pvdbcrTraceRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(pvdbcrTraceRecord);
}
