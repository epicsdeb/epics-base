/* testCreateRequest.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Matej Sekoranja Date: 2010.12.27 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include <pv/pvUnitTest.h>
#include <testMain.h>

#include <pv/current_function.h>
#include <pv/createRequest.h>
#include <pv/bitSet.h>

namespace {

using namespace epics::pvData;
using std::string;
using std::cout;
using std::endl;

static void testCreateRequestInternal() {
    printf("testCreateRequest... \n");
    CreateRequest::shared_pointer  createRequest = CreateRequest::create();
    PVStringPtr pvString;
    string sval;

    string out;
    string request = "";

    PVStructurePtr pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getStructure()->getNumberFields()==0);
    testPass("request %s",request.c_str());

    request = "record[]field()getField()putField()";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("putField").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "record[a=b,x=y]field(a) putField(a),getField(a)";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("record._options.a");
    sval = pvString->get();
    testOk(sval.compare("b")==0,"record.a = b");
    pvString = pvRequest->getSubField<PVString>("record._options.x");
    sval = pvString->get();
    testOk(sval.compare("y")==0,"record.x = y");
    testOk1(pvRequest->getSubField<PVStructure>("field.a").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("putField.a").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.a").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "field(a.b[x=y])";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("field.a.b._options.x");
    sval = pvString->get();
    testOk(sval.compare("y")==0,"field.a.b._options.x = y");
    testPass("request %s",request.c_str());

    request = "field(a.b{c.d})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField("field.a.b.c.d").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "field(a.b[x=y]{c.d})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("field.a.b._options.x");
    sval = pvString->get();
    testOk(sval.compare("y")==0,"field.a.b._options.x = y");
    testOk1(pvRequest->getSubField<PVStructure>("field.a.b.c.d").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "field(a.b[x=y]{c.d[x=y]})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("field.a.b._options.x");
    sval = pvString->get();
    testOk(sval.compare("y")==0,"field.a.b._options.x = y");
    pvString = pvRequest->getSubField<PVString>("field.a.b.c.d._options.x");
    sval = pvString->get();
    testOk(sval.compare("y")==0,"field.a.b.c.d._options.x = y");
    testPass("request %s",request.c_str());

    request = "record[a=b,c=d] field(a.a[a=b]{a.a[a=b]},b.a[a=b]{a,b})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("field.a.a._options.a");
    sval = pvString->get();
    testOk(sval.compare("b")==0,"field.a.a._options.a = b");
    pvString = pvRequest->getSubField<PVString>("field.a.a.a.a._options.a");
    sval = pvString->get();
    testOk(sval.compare("b")==0,"field.a.a.a.a._options.a = b");
    pvString = pvRequest->getSubField<PVString>("field.b.a._options.a");
    sval = pvString->get();
    testOk(sval.compare("b")==0,"field.b.a._options.a = b");
    testOk1(pvRequest->getSubField<PVStructure>("field.b.a.a").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.b.a.b").get()!=NULL);
    testPass("request %s",request.c_str());


    request = "alarm,timeStamp,power.value";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.power.value").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "record[process=true]field(alarm,timeStamp,power.value)";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("record._options.process");
    sval = pvString->get();
    testOk(sval.compare("true")==0,"record._options.process = true");
    testOk1(pvRequest->getSubField<PVStructure>("field.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.power.value").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "record[process=true]field(alarm,timeStamp[algorithm=onChange,causeMonitor=false],power{value,alarm})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("record._options.process");
    sval = pvString->get();
    testOk(sval.compare("true")==0,"record._options.process = true");
    testOk1(pvRequest->getSubField<PVStructure>("field.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.timeStamp").get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("field.timeStamp._options.algorithm");
    sval = pvString->get();
    testOk(sval.compare("onChange")==0,"field.timeStamp._options.algorithm = onChange");
    pvString = pvRequest->getSubField<PVString>("field.timeStamp._options.causeMonitor");
    sval = pvString->get();
    testOk(sval.compare("false")==0,"field.timeStamp._options.causeMonitor = false");
    testOk1(pvRequest->getSubField<PVStructure>("field.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.power.alarm").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "record[int=2,float=3.14159]field(alarm,timeStamp[shareData=true],power.value)";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("record._options.int");
    sval = pvString->get();
    testOk(sval.compare("2")==0,"record._options.int = 2");
    pvString = pvRequest->getSubField<PVString>("record._options.float");
    sval = pvString->get();
    testOk(sval.compare("3.14159")==0,"record._options.float = 3.14159");
    testOk1(pvRequest->getSubField<PVStructure>("field.alarm").get()!=NULL);
    pvString = pvRequest->getSubField<PVString>("field.timeStamp._options.shareData");
    sval = pvString->get();
    testOk(sval.compare("true")==0,"field.timeStamp._options.shareData = true");
    testOk1(pvRequest->getSubField<PVStructure>("field.power.value").get()!=NULL);
    testPass("request %s",request.c_str());

    request = string("record[process=true,xxx=yyy]")
        + "putField(power.value)"
        + "getField(alarm,timeStamp,power{value,alarm},"
        + "current{value,alarm},voltage{value,alarm})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("putField.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.power.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.current.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.current.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.voltage.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.voltage.alarm").get()!=NULL);
    testPass("request %s",request.c_str());

    request = string("field(alarm,timeStamp,supply{")
        + "zero{voltage.value,current.value,power.value},"
        + "one{voltage.value,current.value,power.value}"
        + "})";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.supply.zero.voltage.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.supply.zero.current.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.supply.zero.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.supply.one.voltage.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.supply.one.current.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.supply.one.power.value").get()!=NULL);
    testPass("request %s",request.c_str());

    request = string("record[process=true,xxx=yyy]")
        + "putField(power.value)"
        + "getField(alarm,timeStamp,power{value,alarm},"
        + "current{value,alarm},voltage{value,alarm},"
        + "ps0{alarm,timeStamp,power{value,alarm},current{value,alarm},voltage{value,alarm}},"
        + "ps1{alarm,timeStamp,power{value,alarm},current{value,alarm},voltage{value,alarm}}"
        + ")";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("putField.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.power.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.current.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.current.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.voltage.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.voltage.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.power.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.current.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.current.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.voltage.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps0.voltage.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.timeStamp").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.power.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.power.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.current.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.current.alarm").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.voltage.value").get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("getField.ps1.voltage.alarm").get()!=NULL);
    testPass("request %s",request.c_str());

    request = "a{b{c{d}}}";

    pvRequest = createRequest->createRequest(request);
    if(!pvRequest) { cout<< createRequest->getMessage() << endl;}

    testOk1(pvRequest.get()!=NULL);
    testOk1(pvRequest->getSubField<PVStructure>("field.a.b.c.d").get()!=NULL);
    testPass("request %s",request.c_str());

    request = string("record[process=true,xxx=yyy]")
        + "putField(power.value)"
        + "getField(alarm,timeStamp,power{value,alarm},"
        + "current{value,alarm},voltage{value,alarm},"
        + "ps0{alarm,timeStamp,power{value,alarm},current{value,alarm},voltage{value,alarm}},"
        + "ps1{alarm,timeStamp,power{value,alarm},current{value,alarm},voltage{value,alarm}"
        + ")";

    cout << endl << "Error Expected for next call!!" << endl;
    pvRequest = createRequest->createRequest(request);
    cout << "reason " << createRequest->getMessage() << endl;
    testPass("request %s",request.c_str());

    request = "record[process=true,power.value";

    cout << endl << "Error Expected for next call!!" << endl;
    pvRequest = createRequest->createRequest(request);
    cout << "reason " << createRequest->getMessage() << endl;
    testOk1(pvRequest.get()==NULL);
    testPass("request %s",request.c_str());

    request = "field(alarm.status,alarm.severity)";

    cout << endl << "Error Expected for next call!!" << endl;
    pvRequest = createRequest->createRequest(request);
    cout << "reason " << createRequest->getMessage() << endl;
    testOk1(pvRequest.get()==NULL);
    testPass("request %s",request.c_str());

    request = ":field(record[process=false]power.value)";

    cout << endl << "Error Expected for next call!!" << endl;
    pvRequest = createRequest->createRequest(request);
    cout << "reason " << createRequest->getMessage() << endl;
    testOk1(pvRequest.get()==NULL);
    testPass("request %s",request.c_str());
}

static void testBadRequest()
{
    testThrows(std::runtime_error, createRequest("field("));
    testThrows(std::runtime_error, createRequest("record[field()"));

    CreateRequest::shared_pointer C(CreateRequest::create());
    testOk1(!C->createRequest("field("));
    testDiag("message %s", C->getMessage().c_str());

    testOk1(!!C->createRequest("field(value)"));
    testOk1(C->getMessage().empty());

    // duplicate fieldName C
    // correct is: "field(A,C{D,E.F})"
    testThrows(std::invalid_argument, createRequest("field(A,C.D,C.E.F)"));
}

static
StructureConstPtr maskingType = getFieldCreate()->createFieldBuilder()
        ->add("A", pvInt)
        ->add("B", pvInt)
        ->addNestedStructure("C")
            ->add("D", pvInt)
            ->addNestedStructure("E")
                ->add("F", pvInt)
            ->endNested()
        ->endNested()
        ->createStructure();

static
void testMapper(PVRequestMapper::mode_t mode)
{
    testDiag("=== %s mode==%d", CURRENT_FUNCTION, (int)mode);
    {
        testDiag("Map full structure");

        PVStructurePtr base(getPVDataCreate()->createPVStructure(maskingType));
        PVRequestMapper mapper(*base, *createRequest(""), mode);

        testEqual(mapper.requested(), maskingType);

        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("A")->getFieldOffset())
                  .set(base->getSubFieldT("B")->getFieldOffset())
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.D")->getFieldOffset())
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));

        PVStructurePtr req(getPVDataCreate()->createPVStructure(mapper.requested()));

        base->getSubFieldT<PVInt>("A")->put(1);
        base->getSubFieldT<PVInt>("B")->put(42);

        BitSet output;
        mapper.copyBaseToRequested(*base, BitSet().set(0), *req, output);

        testFieldEqual<PVInt>(req, "A", 1);
        testFieldEqual<PVInt>(req, "B", 42);

        req->getSubFieldT<PVInt>("A")->put(2);
        req->getSubFieldT<PVInt>("B")->put(43);

        mapper.copyBaseFromRequested(*base, output, *req, BitSet().set(0));

        testFieldEqual<PVInt>(req, "A", 2);
        testFieldEqual<PVInt>(req, "B", 43);
        testEqual(mapper.requestedMask(), BitSet().set(0).set(1).set(2).set(3).set(4).set(5).set(6));
    }
    {
        testDiag("Map single leaf field");

        PVStructurePtr base(getPVDataCreate()->createPVStructure(maskingType));
        PVRequestMapper mapper(*base, *createRequest("field(B)"), mode);

        if(mode==PVRequestMapper::Slice)
            testNotEqual(mapper.requested(), maskingType);
        else
            testEqual(mapper.requested(), maskingType);

        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("B")->getFieldOffset()));

        PVStructurePtr req(getPVDataCreate()->createPVStructure(mapper.requested()));

        if(mode==PVRequestMapper::Slice)
            testOk1(!req->getSubField("A"));

        base->getSubFieldT<PVScalar>("A")->putFrom<int32>(11);
        base->getSubFieldT<PVScalar>("B")->putFrom<int32>(42);

        BitSet output;
        mapper.copyBaseToRequested(*base, BitSet().set(0), *req, output);

        if(mode!=PVRequestMapper::Slice)
            testFieldEqual<PVInt>(req, "A", 0);
        testFieldEqual<PVInt>(req, "B", 42);

        req->getSubFieldT<PVScalar>("B")->putFrom<int32>(43);

        mapper.copyBaseFromRequested(*base, output, *req, BitSet().set(0));

        testFieldEqual<PVInt>(req, "B", 43);
        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("B")->getFieldOffset()));

        BitSet cmp;
        mapper.maskBaseToRequested(BitSet().set(base->getSubFieldT("B")->getFieldOffset()), cmp);
        testEqual(cmp, BitSet()
                  .set(req->getSubFieldT("B")->getFieldOffset()));

        cmp.clear();
        mapper.maskBaseFromRequested(cmp, BitSet()
                                   .set(req->getSubFieldT("B")->getFieldOffset()));
        testEqual(cmp, BitSet()
                  .set(base->getSubFieldT("B")->getFieldOffset()));
    }
    {
        testDiag("Map two sub-fields");

        PVStructurePtr base(getPVDataCreate()->createPVStructure(maskingType));
        PVRequestMapper mapper(*base, *createRequest("B,C.D"), mode);

        if(mode==PVRequestMapper::Slice)
            testNotEqual(mapper.requested(), maskingType);
        else
            testEqual(mapper.requested(), maskingType);

        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("B")->getFieldOffset())
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.D")->getFieldOffset()));

        PVStructurePtr req(getPVDataCreate()->createPVStructure(mapper.requested()));

        if(mode==PVRequestMapper::Slice)
            testOk1(!req->getSubField("A"));

        base->getSubFieldT<PVScalar>("A")->putFrom<int32>(11);
        base->getSubFieldT<PVScalar>("B")->putFrom<int32>(1);
        base->getSubFieldT<PVScalar>("C.D")->putFrom<int32>(42);

        BitSet output;
        mapper.copyBaseToRequested(*base, BitSet().set(0), *req, output);

        if(mode!=PVRequestMapper::Slice)
            testFieldEqual<PVInt>(req, "A", 0);
        testFieldEqual<PVInt>(req, "B", 1);
        testFieldEqual<PVInt>(req, "C.D", 42);

        req->getSubFieldT<PVScalar>("B")->putFrom<int32>(2);
        req->getSubFieldT<PVScalar>("C.D")->putFrom<int32>(43);

        mapper.copyBaseFromRequested(*base, output, *req, BitSet().set(0));

        testFieldEqual<PVInt>(req, "B", 2);
        testFieldEqual<PVInt>(req, "C.D", 43);
        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("B")->getFieldOffset())
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.D")->getFieldOffset()));
    }
    {
        testDiag("Map entire sub-structure");

        PVStructurePtr base(getPVDataCreate()->createPVStructure(maskingType));
        PVRequestMapper mapper(*base, *createRequest("field(C.E)"), mode);

        if(mode==PVRequestMapper::Slice)
            testNotEqual(mapper.requested(), maskingType);
        else
            testEqual(mapper.requested(), maskingType);

        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));


        PVStructurePtr req(getPVDataCreate()->createPVStructure(mapper.requested()));

        if(mode==PVRequestMapper::Slice)
            testOk1(!req->getSubField("A"));

        base->getSubFieldT<PVScalar>("A")->putFrom<int32>(11);
        base->getSubFieldT<PVScalar>("C.E.F")->putFrom<int32>(42);

        BitSet output;
        mapper.copyBaseToRequested(*base, BitSet().set(0), *req, output);

        if(mode!=PVRequestMapper::Slice)
            testFieldEqual<PVInt>(req, "A", 0);
        testFieldEqual<PVInt>(req, "C.E.F", 42);

        req->getSubFieldT<PVScalar>("C.E.F")->putFrom<int32>(43);

        mapper.copyBaseFromRequested(*base, output, *req, BitSet().set(0));

        testFieldEqual<PVInt>(req, "C.E.F", 43);
        testEqual(mapper.requestedMask(), BitSet().set(0)
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));

        BitSet cmp;
        mapper.maskBaseToRequested(BitSet()
                                   .set(base->getSubFieldT("C")->getFieldOffset()), cmp);
        testEqual(cmp, BitSet()
                  .set(req->getSubFieldT("C")->getFieldOffset())
                  .set(req->getSubFieldT("C.E")->getFieldOffset())
                  .set(req->getSubFieldT("C.E.F")->getFieldOffset()));

        cmp.clear();
        mapper.maskBaseFromRequested(cmp, BitSet()
                                   .set(req->getSubFieldT("C")->getFieldOffset()));
        testEqual(cmp, BitSet()
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));
    }
}

struct MapperMask {
    PVStructurePtr base, req;
    BitSet bmask, rmask;
    PVRequestMapper mapper;

    MapperMask(PVRequestMapper::mode_t mode) {
        base = getPVDataCreate()->createPVStructure(maskingType);
        mapper.compute(*base, *createRequest("field(B,C.E)"), mode);
        req = getPVDataCreate()->createPVStructure(mapper.requested());
        reset();
    }

    void reset() {
        base->getSubFieldT<PVScalar>("B")->putFrom<int32>(1);
        base->getSubFieldT<PVScalar>("C.E.F")->putFrom<int32>(3);
        req->getSubFieldT<PVScalar>("B")->putFrom<int32>(11);
        req->getSubFieldT<PVScalar>("C.E.F")->putFrom<int32>(13);
    }

    void check(int32 bB, int32 bCEF, int32 rB, int32 rCEF) {
        testFieldEqual<PVInt>(base, "B", bB);
        testFieldEqual<PVInt>(base, "C.E.F", bCEF);
        testFieldEqual<PVInt>(req, "B", rB);
        testFieldEqual<PVInt>(req, "C.E.F", rCEF);
    }

    void testEmptyMaskB2R() {
        mapper.copyBaseToRequested(*base, bmask, *req, rmask);
        check(1, 3, 11, 13);
        testEqual(bmask, BitSet());
        testEqual(rmask, BitSet());
    }

    void testEmptyMaskR2B() {
        mapper.copyBaseFromRequested(*base, bmask, *req, rmask);
        check(1, 3, 11, 13);
        testEqual(bmask, BitSet());
        testEqual(rmask, BitSet());
    }

    void testAllMaskB2R() {
        bmask.set(0);
        mapper.copyBaseToRequested(*base, bmask, *req, rmask);
        check(1, 3, 1, 3);
        testEqual(rmask, BitSet()
                  .set(0)
                  .set(req->getSubFieldT("B")->getFieldOffset())
                  .set(req->getSubFieldT("C")->getFieldOffset())
                  .set(req->getSubFieldT("C.E")->getFieldOffset())
                  .set(req->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testAllMaskR2B() {
        rmask.set(0);
        mapper.copyBaseFromRequested(*base, bmask, *req, rmask);
        check(11, 13, 11, 13);
        testEqual(bmask, BitSet()
                  .set(0)
                  .set(base->getSubFieldT("B")->getFieldOffset())
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testMaskOneB2R() {
        bmask.set(base->getSubFieldT("B")->getFieldOffset());
        mapper.copyBaseToRequested(*base, bmask, *req, rmask);
        check(1, 3, 1, 13);
        testEqual(rmask, BitSet()
                  .set(req->getSubFieldT("B")->getFieldOffset()));
    }

    void testMaskOneR2B() {
        rmask.set(req->getSubFieldT("B")->getFieldOffset());
        mapper.copyBaseFromRequested(*base, bmask, *req, rmask);
        check(11, 3, 11, 13);
        testEqual(bmask, BitSet()
                  .set(base->getSubFieldT("B")->getFieldOffset()));
    }

    void testMaskOtherB2R() {
        bmask.set(base->getSubFieldT("C.E.F")->getFieldOffset());
        mapper.copyBaseToRequested(*base, bmask, *req, rmask);
        check(1, 3, 11, 3);
        testEqual(rmask, BitSet()
                  .set(req->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testMaskOtherR2B() {
        rmask.set(req->getSubFieldT("C.E.F")->getFieldOffset());
        mapper.copyBaseFromRequested(*base, bmask, *req, rmask);
        check(1, 13, 11, 13);
        testEqual(bmask, BitSet()
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testMaskSub1B2R() {
        bmask.set(base->getSubFieldT("C.E")->getFieldOffset());
        mapper.copyBaseToRequested(*base, bmask, *req, rmask);
        check(1, 3, 11, 3);
        testEqual(rmask, BitSet()
                  .set(req->getSubFieldT("C.E")->getFieldOffset())
                  .set(req->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testMaskSub1R2B() {
        rmask.set(req->getSubFieldT("C.E")->getFieldOffset());
        mapper.copyBaseFromRequested(*base, bmask, *req, rmask);
        check(1, 13, 11, 13);
        testEqual(bmask, BitSet()
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testMaskSub2B2R() {
        bmask.set(base->getSubFieldT("C")->getFieldOffset());
        mapper.copyBaseToRequested(*base, bmask, *req, rmask);
        check(1, 3, 11, 3);
        testEqual(rmask, BitSet()
                  .set(req->getSubFieldT("C")->getFieldOffset())
                  .set(req->getSubFieldT("C.E")->getFieldOffset())
                  .set(req->getSubFieldT("C.E.F")->getFieldOffset()));
    }

    void testMaskSub2R2B() {
        rmask.set(req->getSubFieldT("C")->getFieldOffset());
        mapper.copyBaseFromRequested(*base, bmask, *req, rmask);
        check(1, 13, 11, 13);
        testEqual(bmask, BitSet()
                  .set(base->getSubFieldT("C")->getFieldOffset())
                  .set(base->getSubFieldT("C.E")->getFieldOffset())
                  .set(base->getSubFieldT("C.E.F")->getFieldOffset()));
    }
};

void testMaskWarn()
{
    PVStructurePtr base(getPVDataCreate()->createPVStructure(maskingType));
    PVRequestMapper mapper(*base, *createRequest("field(B,invalid)"), PVRequestMapper::Slice);

    testEqual(mapper.warnings(), "No field 'invalid' ");
}

void testMaskErr()
{
    PVStructurePtr base(getPVDataCreate()->createPVStructure(maskingType));
    testThrows(std::runtime_error, PVRequestMapper mapper(*base, *createRequest("field(invalid)"), PVRequestMapper::Slice));
}

} // namespace

MAIN(testCreateRequest)
{
    testPlan(315);
    testCreateRequestInternal();
    testBadRequest();
    testMapper(PVRequestMapper::Slice);
    testMapper(PVRequestMapper::Mask);
#undef TEST_METHOD
#define TEST_METHOD(KLASS, METHOD) \
    { \
        testDiag("------- %s::%s Mask --------", #KLASS, #METHOD); \
        { KLASS inst(PVRequestMapper::Mask); inst.METHOD(); } \
        testDiag("------- %s::%s Slice --------", #KLASS, #METHOD); \
        { KLASS inst(PVRequestMapper::Slice); inst.METHOD(); } \
    }
    TEST_METHOD(MapperMask, testEmptyMaskB2R);
    TEST_METHOD(MapperMask, testEmptyMaskR2B);
    TEST_METHOD(MapperMask, testAllMaskB2R);
    TEST_METHOD(MapperMask, testAllMaskR2B);
    TEST_METHOD(MapperMask, testMaskOneB2R);
    TEST_METHOD(MapperMask, testMaskOneR2B);
    TEST_METHOD(MapperMask, testMaskOtherB2R);
    TEST_METHOD(MapperMask, testMaskOtherR2B);
    TEST_METHOD(MapperMask, testMaskSub1B2R);
    TEST_METHOD(MapperMask, testMaskSub1R2B);
    TEST_METHOD(MapperMask, testMaskSub2B2R);
    TEST_METHOD(MapperMask, testMaskSub2R2B);
    testMaskWarn();
    testMaskErr();
    return testDone();
}
