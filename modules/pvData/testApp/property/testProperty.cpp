/* testProperty.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Marty Kraimer Date: 2010.11 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstring>
#include <cstdio>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/alarm.h>
#include <pv/control.h>
#include <pv/display.h>
#include <pv/timeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvControl.h>
#include <pv/pvDisplay.h>
#include <pv/pvEnumerated.h>
#include <pv/pvTimeStamp.h>

using namespace epics::pvData;
using std::string;

static FieldCreatePtr fieldCreate;
static PVDataCreatePtr pvDataCreate;
static StandardFieldPtr standardField;
static StandardPVFieldPtr standardPVField;
static string alarmTimeStamp("alarm,timeStamp");
static string allProperties("alarm,timeStamp,display,control");

static PVStructurePtr doubleRecord;
static PVStructurePtr enumeratedRecord;

static void printRecords()
{
    std::cout << "# doubleRecord" << std::endl << *doubleRecord << std::endl;
    std::cout << "# enumeratedRecord" << std::endl << *enumeratedRecord << std::endl;
}

static void createRecords()
{
    doubleRecord = standardPVField->scalar(pvDouble,allProperties);
    StringArray choices;
    choices.reserve(4);
    choices.push_back("1");
    choices.push_back("2");
    choices.push_back("3");
    choices.push_back("4");
    enumeratedRecord = standardPVField->enumerated(choices,alarmTimeStamp);
}

static void testAlarm()
{
    testDiag("testAlarm\n");
    Alarm alarm;
    PVAlarm pvAlarm;
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField<PVStructure>(string("alarm"));
    if(pvField.get()==NULL) {
        testFail("testAlarm ERROR did not find field alarm\n");
        return;
    }
    result = pvAlarm.attach(pvField);
    testOk1(result);
    Alarm al;
    al.setMessage(string("testMessage"));
    al.setSeverity(majorAlarm);
    al.setStatus(clientStatus);
    result = pvAlarm.set(al);
    testOk1(result);
    pvAlarm.get(alarm);
    testOk1(al.getMessage().compare(alarm.getMessage())==0);
    testOk1(al.getSeverity()==alarm.getSeverity());
    testOk1(al.getStatus()==alarm.getStatus());
    string message = alarm.getMessage();
    string severity = (*AlarmSeverityFunc::getSeverityNames())[alarm.getSeverity()];
    string status = (*AlarmStatusFunc::getStatusNames())[alarm.getStatus()];
    testPass("testAlarm PASSED");
}

static void testTimeStamp()
{
    testDiag("testTimeStamp\n");
    TimeStamp timeStamp;
    PVTimeStamp pvTimeStamp;
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField<PVStructure>(string("timeStamp"));
    if(pvField.get()==NULL) {
        testFail("testTimeStamp ERROR did not find field timeStamp");
        return;
    }
    result = pvTimeStamp.attach(pvField);
    testOk1(result);
    TimeStamp ts;
    ts.getCurrent();
    ts.setUserTag(32);
    result = pvTimeStamp.set(ts);
    testOk1(result);
    pvTimeStamp.get(timeStamp);
    testOk1(ts.getSecondsPastEpoch()==timeStamp.getSecondsPastEpoch());
    testOk1(ts.getNanoseconds()==timeStamp.getNanoseconds());
    testOk1(ts.getUserTag()==timeStamp.getUserTag());
    time_t tt;
    timeStamp.toTime_t(tt);
    struct tm ctm;
    memcpy(&ctm,localtime(&tt),sizeof(struct tm));
    testDiag(
                "%4.4d.%2.2d.%2.2d %2.2d:%2.2d:%2.2d %d nanoseconds isDst %s userTag %d",
                ctm.tm_year+1900,ctm.tm_mon + 1,ctm.tm_mday,
                ctm.tm_hour,ctm.tm_min,ctm.tm_sec,
                (int)timeStamp.getNanoseconds(),
                (ctm.tm_isdst==0) ? "false" : "true",
                (int)timeStamp.getUserTag());

    timeStamp.put(0,0);
    pvTimeStamp.set(timeStamp);
    testPass("testTimeStamp PASSED\n");
}

static void testControl()
{
    testDiag("testControl\n");
    Control control;
    PVControl pvControl;
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField<PVStructure>(string("control"));
    if(pvField.get()==NULL) {
        testFail("testControl ERROR did not find field control\n");
        return;
    }
    result = pvControl.attach(pvField);
    testOk1(result);
    Control cl;
    cl.setLow(1.0);
    cl.setHigh(10.0);
    result = pvControl.set(cl);
    testOk1(result);
    pvControl.get(control);
    testOk1(cl.getLow()==control.getLow());
    testOk1(cl.getHigh()==control.getHigh());
    double low = control.getLow();
    double high = control.getHigh();
    testDiag(" low %f high %f\n",low,high);
    testPass("testControl PASSED\n");
}

static void testDisplay()
{
    testDiag("testDisplay\n");
    Display display;
    PVDisplay pvDisplay;
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField<PVStructure>(string("display"));
    if(pvField.get()==NULL) {
        testFail("testDisplay ERROR did not find field display\n");
        return;
    }
    result = pvDisplay.attach(pvField);
    testOk1(result);
    Display dy;
    dy.setLow(-10.0);
    dy.setHigh(-1.0);
    dy.setDescription(string("testDescription"));
    dy.setFormat(string("%f10.0"));
    dy.setUnits(string("volts"));
    result = pvDisplay.set(dy);
    testOk1(result);
    pvDisplay.get(display);
    testOk1(dy.getLow()==display.getLow());
    testOk1(dy.getHigh()==display.getHigh());
    testOk1(dy.getDescription().compare(display.getDescription())==0);
    testOk1(dy.getFormat().compare(display.getFormat())==0);
    testOk1(dy.getUnits().compare(display.getUnits())==0);
    double low = display.getLow();
    double high = display.getHigh();
    testDiag(" low %f high %f\n",low,high);
    testPass("testDisplay PASSED\n");
}

static void testEnumerated()
{
    testDiag("testEnumerated\n");
    PVEnumerated pvEnumerated;
    bool result;
    PVFieldPtr pvField = enumeratedRecord->getSubField<PVStructure>(string("value"));
    if(pvField.get()==NULL) {
        testFail("testEnumerated ERROR did not find field enumerated\n");
        return;
    }
    result = pvEnumerated.attach(pvField);
    testOk1(result);
    int32 index = pvEnumerated.getIndex();
    string choice = pvEnumerated.getChoice();
    PVStringArray::const_svector choices = pvEnumerated.getChoices();
    int32 numChoices = pvEnumerated.getNumberChoices();
    testDiag("index %d choice %s choices",(int)index,choice.c_str());
    for(int i=0; i<numChoices; i++ )
        testDiag(" %s",choices[i].c_str());

    pvEnumerated.setIndex(2);
    index = pvEnumerated.getIndex();
    choice = pvEnumerated.getChoice();
    testDiag("index %d choice %s\n",(int)index,choice.c_str());
    testPass("testEnumerated PASSED\n");
}

MAIN(testProperty)
{
    testPlan(27);
    testDiag("Tests property");
    fieldCreate = getFieldCreate();
    pvDataCreate = getPVDataCreate();
    standardField = getStandardField();
    standardPVField = getStandardPVField();
    createRecords();
    testAlarm();
    testTimeStamp();
    testControl();
    testDisplay();
    testEnumerated();
    printRecords();
    return testDone();;
}
