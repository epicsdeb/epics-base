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

// The following must be the last include for code pvDatabase implements
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvDatabase.h"
#include "pv/pvdbcrScalarRecord.h"
using namespace epics::pvData;
using namespace std;

namespace epics { namespace pvDatabase {

PvdbcrScalarRecord::PvdbcrScalarRecord(
    std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup)
: PVRecord(recordName,pvStructure,asLevel,asGroup)
{}

PvdbcrScalarRecordPtr PvdbcrScalarRecord::create(
    std::string const & recordName,std::string const &  scalarType,
    int asLevel,std::string const & asGroup)
{
    ScalarType st = epics::pvData::ScalarTypeFunc::getScalarType(scalarType);
    FieldCreatePtr fieldCreate = getFieldCreate();
    StandardFieldPtr standardField = getStandardField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr top = fieldCreate->createFieldBuilder()->
        add("value",st) ->
        add("timeStamp",standardField->timeStamp()) ->
        add("alarm",standardField->alarm()) ->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(top);   
    PvdbcrScalarRecordPtr pvRecord(new PvdbcrScalarRecord(recordName,pvStructure,asLevel,asGroup));
    pvRecord->initPVRecord();
    return pvRecord;
};
}}

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg arg1 = { "scalarType", iocshArgString };
static const iocshArg arg2 = { "asLevel", iocshArgInt };
static const iocshArg arg3 = { "asGroup", iocshArgString };
static const iocshArg *args[] = {&arg0,&arg1,&arg2,&arg3};

static const iocshFuncDef pvdbcrScalarFuncDef = {"pvdbcrScalarRecord", 4,args};

static void pvdbcrScalarCallFunc(const iocshArgBuf *args)
{
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrScalarRecord recordName not specified");
    }
    string recordName = string(sval);
    sval = args[1].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrScalarRecord scalarType not specified");
    }
    string scalarType = string(sval);
    int asLevel = args[2].ival;
    string asGroup("DEFAULT");
    sval = args[3].sval;
    if(sval) {
        asGroup = string(sval);
    }
    epics::pvDatabase::PvdbcrScalarRecordPtr record
        = epics::pvDatabase::PvdbcrScalarRecord::create(recordName,scalarType);
    epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
    record->setAsLevel(asLevel);
    record->setAsGroup(asGroup);
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void pvdbcrScalarRecord(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdbcrScalarFuncDef, pvdbcrScalarCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(pvdbcrScalarRecord);
}
