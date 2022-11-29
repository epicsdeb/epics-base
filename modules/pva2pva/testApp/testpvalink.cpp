
#include <dbUnitTest.h>
#include <testMain.h>
#include <int64inRecord.h>
#include <int64outRecord.h>

#include <pv/qsrv.h>
#include "utilities.h"
#include "pvalink.h"
#include "pv/qsrv.h"

namespace {

void testGet()
{
    testDiag("==== testGet ====");

    int64inRecord *i1 = (int64inRecord*)testdbRecordPtr("src:i1");

    while(!dbIsLinkConnected(&i1->inp))
        testqsrvWaitForLinkEvent(&i1->inp);

    testdbGetFieldEqual("target:i.VAL", DBF_INT64, 42LL);

    testdbGetFieldEqual("src:i1.VAL", DBF_INT64, 0LL); // value before first process

    testdbGetFieldEqual("src:i1.INP", DBF_STRING, "{\"pva\":\"target:i\"}");

    testdbPutFieldOk("src:i1.PROC", DBF_INT64, 1LL);

    testdbGetFieldEqual("src:i1.VAL", DBF_INT64, 42LL);

    testdbPutFieldOk("src:i1.INP", DBF_STRING, "{\"pva\":\"target:ai\"}");

    while(!dbIsLinkConnected(&i1->inp))
        testqsrvWaitForLinkEvent(&i1->inp);

    testdbGetFieldEqual("src:i1.VAL", DBF_INT64, 42LL); // changing link doesn't automatically process

    testdbPutFieldOk("src:i1.PROC", DBF_INT64, 1LL);

    testdbGetFieldEqual("src:i1.VAL", DBF_INT64, 4LL); // now it's changed
}

void testPut()
{
    testDiag("==== testPut ====");

    int64outRecord *o2 = (int64outRecord*)testdbRecordPtr("src:o2");

    while(!dbIsLinkConnected(&o2->out))
        testqsrvWaitForLinkEvent(&o2->out);

    testdbGetFieldEqual("target:i2.VAL", DBF_INT64, 43LL);
    testdbGetFieldEqual("src:o2.VAL", DBF_INT64, 0LL);
    testdbGetFieldEqual("src:o2.OUT", DBF_STRING, "{\"pva\":\"target:i2\"}");

    testdbPutFieldOk("src:o2.VAL", DBF_INT64, 14LL);

    testdbGetFieldEqual("target:i2.VAL", DBF_INT64, 14LL);
    testdbGetFieldEqual("src:o2.VAL", DBF_INT64, 14LL);
}

void testPutAsync()
{
#ifdef USE_MULTILOCK
    testDiag("==== testPutAsync ====");

    int64outRecord *trig = (int64outRecord*)testdbRecordPtr("async:trig");

    while(!dbIsLinkConnected(&trig->out))
        testqsrvWaitForLinkEvent(&trig->out);

    testMonitor* done = testMonitorCreate("async:after", DBE_VALUE, 0);

    testdbPutFieldOk("async:trig.PROC", DBF_LONG, 1);
    testMonitorWait(done);

    testdbGetFieldEqual("async:trig",  DBF_LONG, 1);
    testdbGetFieldEqual("async:slow",  DBF_LONG, 1); // pushed from async:trig
    testdbGetFieldEqual("async:slow2", DBF_LONG, 2);
    testdbGetFieldEqual("async:after", DBF_LONG, 3);

#else
    testSkip(5, "Not USE_MULTILOCK");
#endif
}

} // namespace

extern "C"
void pvaLinkTestIoc_registerRecordDeviceDriver(struct dbBase *);

MAIN(testpvalink)
{
    testPlan(20);

    // Disable PVA client provider, use local/QSRV provider
    pvaLinkIsolate = 1;

    try {
        TestIOC IOC;

        testdbReadDatabase("pvaLinkTestIoc.dbd", NULL, NULL);
        pvaLinkTestIoc_registerRecordDeviceDriver(pdbbase);
        testdbReadDatabase("testpvalink.db", NULL, NULL);

        IOC.init();
        testGet();
        testPut();
        testPutAsync();
        testqsrvShutdownOk();
        IOC.shutdown();
        testqsrvCleanup();

    }catch(std::exception& e){
        testFail("Unexpected exception: %s", e.what());
    }
    // call epics atexits explicitly as workaround for c++ static dtor issues...
    epicsExit(testDone());
}
