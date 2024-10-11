/* StandardField.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mrk
 */
#include <string>
#include <cstdio>
#include <stdexcept>

#include <epicsMutex.h>
#include <epicsThread.h>

#define epicsExportSharedSymbols
#include <pv/lock.h>
#include <pv/pvIntrospect.h>
#include <pv/standardField.h>

using std::tr1::static_pointer_cast;
using std::string;

namespace epics { namespace pvData {

static
StructureConstPtr buildValueAlarm(ScalarType vtype)
{
    return FieldBuilder::begin()
            ->setId("valueAlarm_t")
            ->add("active", pvBoolean)
            ->add("lowAlarmLimit", vtype)
            ->add("lowWarningLimit", vtype)
            ->add("highWarningLimit", vtype)
            ->add("highAlarmLimit", vtype)
            ->add("lowAlarmSeverity", pvInt)
            ->add("lowWarningSeverity", pvInt)
            ->add("highWarningSeverity", pvInt)
            ->add("highAlarmSeverity", pvInt)
            ->add("hysteresis", pvByte)
            ->createStructure();
}

StandardField::StandardField()
    :fieldCreate(getFieldCreate())
    ,notImplemented("not implemented")
    ,valueFieldName("value")

    ,alarmField(FieldBuilder::begin()
                ->setId("alarm_t")
                ->add("severity", pvInt)
                ->add("status", pvInt)
                ->add("message", pvString)
                ->createStructure())

    ,timeStampField(FieldBuilder::begin()
                    ->setId("time_t")
                    ->add("secondsPastEpoch", pvLong)
                    ->add("nanoseconds", pvInt)
                    ->add("userTag", pvInt)
                    ->createStructure())

    ,displayField(FieldBuilder::begin()
                  ->setId("display_t")
                  ->add("limitLow", pvDouble)
                  ->add("limitHigh", pvDouble)
                  ->add("description", pvString)
                  ->add("format", pvString)
                  ->add("units", pvString)
                  ->createStructure())

    ,controlField(FieldBuilder::begin()
                  ->setId("control_t")
                  ->add("limitLow", pvDouble)
                  ->add("limitHigh", pvDouble)
                  ->add("minStep", pvDouble)
                  ->createStructure())

    ,booleanAlarmField(FieldBuilder::begin()
                       ->setId("valueAlarm_t")
                       ->add("active", pvBoolean)
                       ->add("falseSeverity", pvInt)
                       ->add("trueSeverity", pvInt)
                       ->add("changeStateSeverity", pvInt)
                       ->createStructure())

    ,byteAlarmField(buildValueAlarm(pvByte))
    ,shortAlarmField(buildValueAlarm(pvShort))
    ,intAlarmField(buildValueAlarm(pvInt))
    ,longAlarmField(buildValueAlarm(pvLong))
    ,ubyteAlarmField(buildValueAlarm(pvUByte))
    ,ushortAlarmField(buildValueAlarm(pvUShort))
    ,uintAlarmField(buildValueAlarm(pvUInt))
    ,ulongAlarmField(buildValueAlarm(pvULong))
    ,floatAlarmField(buildValueAlarm(pvFloat))
    ,doubleAlarmField(buildValueAlarm(pvDouble))

    ,enumeratedAlarmField(FieldBuilder::begin()
                          ->setId("valueAlarm_t")
                          ->add("active", pvBoolean)
                          ->add("stateSeverity", pvInt)
                          ->add("changeStateSeverity", pvInt)
                          ->createStructure())
{}

StandardField::~StandardField(){}

StructureConstPtr StandardField::createProperties(string id,FieldConstPtr field,string properties)
{
    bool gotAlarm = false;
    bool gotTimeStamp = false;
    bool gotDisplay = false;
    bool gotControl = false;
    bool gotValueAlarm = false;
    int numProp = 0;
    if(properties.find("alarm")!=string::npos) { gotAlarm = true; numProp++; }
    if(properties.find("timeStamp")!=string::npos) { gotTimeStamp = true; numProp++; }
    if(properties.find("display")!=string::npos) { gotDisplay = true; numProp++; }
    if(properties.find("control")!=string::npos) { gotControl = true; numProp++; }
    if(properties.find("valueAlarm")!=string::npos) { gotValueAlarm = true; numProp++; }
    StructureConstPtr valueAlarm;
    Type type= field->getType();
    while(gotValueAlarm) {
        if(type==epics::pvData::scalar || type==epics::pvData::scalarArray) {
           ScalarType scalarType = (type==epics::pvData::scalar) ?
                static_pointer_cast<const Scalar>(field)->getScalarType() :
                static_pointer_cast<const ScalarArray>(field)->getElementType();
           switch(scalarType) {
               case pvBoolean: valueAlarm = booleanAlarmField; break;
               case pvByte: valueAlarm = byteAlarmField; break;
               case pvShort: valueAlarm = shortAlarmField; break;
               case pvInt: valueAlarm = intAlarmField; break;
               case pvLong: valueAlarm = longAlarmField; break;
               case pvUByte: valueAlarm = ubyteAlarmField; break;
               case pvUShort: valueAlarm = ushortAlarmField; break;
               case pvUInt: valueAlarm = uintAlarmField; break;
               case pvULong: valueAlarm = ulongAlarmField; break;
               case pvFloat: valueAlarm = floatAlarmField; break;
               case pvDouble: valueAlarm = doubleAlarmField; break;
               case pvString:
                throw std::logic_error(string("valueAlarm property not supported for pvString"));
           }
           break;
        }
        if(type==structure) {
            StructureConstPtr structurePtr = static_pointer_cast<const Structure>(field);
            StringArray const & names = structurePtr->getFieldNames();
            if(names.size()==2) {
                FieldConstPtrArray const & fields = structurePtr->getFields();
                FieldConstPtr first = fields[0];
                FieldConstPtr second = fields[1];
                string nameFirst = names[0];
                string nameSecond = names[1];
                int compareFirst = nameFirst.compare("index");
                int compareSecond = nameSecond.compare("choices");
                if(compareFirst==0 && compareSecond==0) {
                    if(first->getType()==epics::pvData::scalar
                    && second->getType()==epics::pvData::scalarArray) {
                        ScalarConstPtr scalarFirst = static_pointer_cast<const Scalar>(first);
                        ScalarArrayConstPtr scalarArraySecond =
                            static_pointer_cast<const ScalarArray>(second);
                        if(scalarFirst->getScalarType()==pvInt
                        && scalarArraySecond->getElementType()==pvString) {
                            valueAlarm = enumeratedAlarmField;
                            break;
                        }
                    }
                }
            }
        }
        throw std::logic_error(string("valueAlarm property for illegal type"));
    }
    size_t numFields = numProp+1;
    FieldConstPtrArray fields(numFields);
    StringArray names(numFields);
    int next = 0;
    names[0] = "value";
    fields[next++] = field;
    if(gotAlarm) {
        names[next] = "alarm";
        fields[next++] = alarmField;
    }
    if(gotTimeStamp) {
        names[next] = "timeStamp";
        fields[next++] = timeStampField;
    }
    if(gotDisplay) {
        names[next] = "display";
        fields[next++] = displayField;
    }
    if(gotControl) {
        names[next] = "control";
        fields[next++] = controlField;
    }
    if(gotValueAlarm) {
        names[next] = "valueAlarm";
        fields[next++] = valueAlarm;
    }
    return fieldCreate->createStructure(id,names,fields);
}

StructureConstPtr StandardField::scalar(
    ScalarType type,string  const &properties)
{
    ScalarConstPtr field = fieldCreate->createScalar(type); // scalar_t
    return createProperties("epics:nt/NTScalar:1.0",field,properties);
}

StructureConstPtr StandardField::regUnion(
    UnionConstPtr const &field,
        string const & properties)
{
   return createProperties("epics:nt/NTUnion:1.0",field,properties);
}

StructureConstPtr StandardField::variantUnion(
    string const & properties)
{
    UnionConstPtr field =  fieldCreate->createVariantUnion();
    return createProperties("epics:nt/NTUnion:1.0",field,properties);
}

StructureConstPtr StandardField::scalarArray(
    ScalarType elementType, string  const &properties)
{
    ScalarArrayConstPtr field = fieldCreate->createScalarArray(elementType); // scalar_t[]
    return createProperties("epics:nt/NTScalarArray:1.0",field,properties);
}


StructureConstPtr StandardField::structureArray(
    StructureConstPtr const & structure,string  const &properties)
{
    StructureArrayConstPtr field = fieldCreate->createStructureArray(
        structure);
    return createProperties("epics:nt/NTStructureArray:1.0",field,properties);
}

StructureConstPtr StandardField::unionArray(
    UnionConstPtr const & punion,string  const &properties)
{
    UnionArrayConstPtr field = fieldCreate->createUnionArray(
        punion);
    return createProperties("epics:nt/NTUnionArray:1.0",field,properties);
}

StructureConstPtr StandardField::enumerated()
{
    size_t num = 2;
    FieldConstPtrArray fields(num);
    StringArray names(num);
    names[0] = "index";
    names[1] = "choices";
    fields[0] = fieldCreate->createScalar(pvInt);
    fields[1] = fieldCreate->createScalarArray(pvString);
    return fieldCreate->createStructure("enum_t",names,fields);
    // NOTE: if this method is used to get NTEnum without properties the ID will be wrong!
}

StructureConstPtr StandardField::enumerated(string  const &properties)
{
    StructureConstPtr field = enumerated(); // enum_t
    return createProperties("epics:nt/NTEnum:1.0",field,properties);
}

static StandardFieldPtr *stdFieldGbl;

static epicsThreadOnceId stdFieldGblOnce = EPICS_THREAD_ONCE_INIT;

void StandardField::once(void*)
{
    stdFieldGbl = new StandardFieldPtr;
    stdFieldGbl->reset(new StandardField);
}

const StandardFieldPtr &StandardField::getStandardField()
{
    epicsThreadOnce(&stdFieldGblOnce, &StandardField::once, 0);

    return *stdFieldGbl;
}

}}
