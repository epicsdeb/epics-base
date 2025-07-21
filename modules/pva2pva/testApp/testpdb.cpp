
#include <testMain.h>

#include <iocsh.h>
#include <epicsAtomic.h>
#include <dbAccess.h>
#include <pva/client.h>

#include <pv/reftrack.h>
#include <pv/epicsException.h>

#include "utilities.h"
#include "pvif.h"
#include "pdb.h"
#include "pdbsingle.h"
#ifdef USE_MULTILOCK
#  include "pdbgroup.h"
#endif

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

namespace {

pvd::PVStructurePtr makeRequest(bool atomic)
{    pvd::StructureConstPtr def(pvd::getFieldCreate()->createFieldBuilder()
                                ->addNestedStructure("record")
                                    ->addNestedStructure("_options")
                                        ->add("atomic", pvd::pvBoolean)
                                        ->endNested()
                                    ->endNested()
                                ->createStructure());
     pvd::PVStructurePtr pvr(pvd::getPVDataCreate()->createPVStructure(def));
     pvr->getSubFieldT<pvd::PVBoolean>("record._options.atomic")->put(atomic);
    return pvr;
}

void testSingleGet(pvac::ClientProvider& client)
{
    testDiag("test single get");
    pvd::PVStructure::const_shared_pointer value(client.connect("rec1").get());

    testFieldEqual<pvd::PVDouble>(value, "value", 1.0);
    testFieldEqual<pvd::PVDouble>(value, "display.limitHigh", 100.0);
    testFieldEqual<pvd::PVDouble>(value, "display.limitLow", -100.0);

    value = client.connect("rec1.RVAL").get();
    testFieldEqual<pvd::PVInt>(value, "value", 10);
}

void testGroupGet(pvac::ClientProvider& client)
{
    testDiag("test group get");
#ifdef USE_MULTILOCK
    pvd::PVStructure::const_shared_pointer value;

    testDiag("get non-atomic");

    value = client.connect("grp1").get(3.0, makeRequest(false));
    testFieldEqual<pvd::PVDouble>(value, "fld1.value", 3.0);
    testFieldEqual<pvd::PVInt>(value,    "fld2.value", 30);
    testFieldEqual<pvd::PVDouble>(value, "fld3.value", 4.0);
    testFieldEqual<pvd::PVInt>(value,    "fld4.value", 40);

    testDiag("get atomic");
    value = client.connect("grp1").get(3.0, makeRequest(true));
    testFieldEqual<pvd::PVDouble>(value, "fld1.value", 3.0);
    testFieldEqual<pvd::PVInt>(value,    "fld2.value", 30);
    testFieldEqual<pvd::PVDouble>(value, "fld3.value", 4.0);
    testFieldEqual<pvd::PVInt>(value,    "fld4.value", 40);
#else
    testSkip(8, "No multilock");
#endif
}

void testSinglePut(pvac::ClientProvider& client)
{
    testDiag("test single put");

    testdbPutFieldOk("rec1", DBR_DOUBLE, 1.0);

    client.connect("rec1.VAL").put().set("value", 2.0).exec();

    testdbGetFieldEqual("rec1", DBR_DOUBLE, 2.0);
}

void testGroupPut(pvac::ClientProvider& client)
{
    testDiag("test group put");
#ifdef USE_MULTILOCK

    testdbPutFieldOk("rec3", DBR_DOUBLE, 3.0);
    testdbPutFieldOk("rec4", DBR_DOUBLE, 4.0);
    testdbPutFieldOk("rec3.RVAL", DBR_LONG, 30);
    testdbPutFieldOk("rec4.RVAL", DBR_LONG, 40);

    // ignored for lack of +putorder
    client.connect("grp1").put().set("fld2.value", 111).exec();

    testdbPutFieldOk("rec3", DBR_DOUBLE, 3.0);
    testdbPutFieldOk("rec4", DBR_DOUBLE, 4.0);
    testdbPutFieldOk("rec3.RVAL", DBR_LONG, 30);
    testdbPutFieldOk("rec4.RVAL", DBR_LONG, 40);

    client.connect("grp1").put().set("fld3.value", 5.0).exec();

    testdbGetFieldEqual("rec3", DBR_DOUBLE, 3.0);
    testdbGetFieldEqual("rec4", DBR_DOUBLE, 5.0);
    testdbGetFieldEqual("rec3.RVAL", DBR_LONG, 30);
    testdbGetFieldEqual("rec4.RVAL", DBR_LONG, 40);
#else
    testSkip(12, "No multilock");
#endif
}

void testSingleMonitor(pvac::ClientProvider& client)
{
    testDiag("test single monitor");

    testdbPutFieldOk("rec1", DBR_DOUBLE, 1.0);

    testDiag("subscribe to rec1.VAL");
    pvac::MonitorSync mon(client.connect("rec1").monitor());

    testOk1(mon.wait(3.0));
    testDiag("Initial event");
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

    testOk1(mon.changed.get(0));
    testFieldEqual<pvd::PVDouble>(mon.root, "value", 1.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "display.limitHigh", 100.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "display.limitLow", -100.0);

    testOk1(!mon.poll());

    testDiag("trigger new VALUE event");
    testdbPutFieldOk("rec1", DBR_DOUBLE, 11.0);

    testDiag("Wait for event");
    testOk1(mon.wait(3.0));
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

#ifdef HAVE_UTAG
    testTrue(mon.changed.get(mon.root->getSubFieldT("timeStamp.userTag")->getFieldOffset()));
    mon.changed.clear(mon.root->getSubFieldT("timeStamp.userTag")->getFieldOffset());
#else
    testSkip(1, "!HAVE_UTAG");
#endif
    testEqual(mon.changed, pvd::BitSet()
              .set(mon.root->getSubFieldT("value")->getFieldOffset())
              .set(mon.root->getSubFieldT("alarm.severity")->getFieldOffset())
              .set(mon.root->getSubFieldT("alarm.status")->getFieldOffset())
              .set(mon.root->getSubFieldT("alarm.message")->getFieldOffset())
              .set(mon.root->getSubFieldT("timeStamp.secondsPastEpoch")->getFieldOffset())
              .set(mon.root->getSubFieldT("timeStamp.nanoseconds")->getFieldOffset()));

    testFieldEqual<pvd::PVDouble>(mon.root, "value", 11.0);

    testOk1(!mon.poll());

    testDiag("trigger new PROPERTY event");
    testdbPutFieldOk("rec1.HOPR", DBR_DOUBLE, 50.0);

    testDiag("Wait for event");
    testOk1(mon.wait(3.0));
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

    testOk1(mon.changed.get(mon.root->getSubFieldT("display.limitHigh")->getFieldOffset()));
    testOk1(mon.changed.get(mon.root->getSubFieldT("display.limitLow")->getFieldOffset()));
    testFieldEqual<pvd::PVDouble>(mon.root, "display.limitHigh", 50.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "display.limitLow", -100.0);

    testOk1(!mon.poll());
}

void testGroupMonitor(pvac::ClientProvider& client)
{
    testDiag("test group monitor");
#ifdef USE_MULTILOCK

    testdbPutFieldOk("rec3", DBR_DOUBLE, 3.0);
    testdbPutFieldOk("rec4", DBR_DOUBLE, 4.0);
    testdbPutFieldOk("rec3.RVAL", DBR_LONG, 30);
    testdbPutFieldOk("rec4.RVAL", DBR_LONG, 40);

    testDiag("subscribe to grp1");
    pvac::MonitorSync mon(client.connect("grp1").monitor());


    testDiag("Wait for initial event");
    testOk1(mon.wait(3.0));
    testDiag("Initial event");
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

    testFieldEqual<pvd::PVDouble>(mon.root, "fld1.value", 3.0);
    testFieldEqual<pvd::PVInt>(mon.root,    "fld2.value", 30);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld3.value", 4.0);
    testFieldEqual<pvd::PVInt>(mon.root,    "fld4.value", 40);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld1.display.limitHigh", 200.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld1.display.limitLow", -200.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld2.display.limitHigh", 2147483647.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld2.display.limitLow", -2147483648.0);

    testOk1(!mon.poll());

    testdbPutFieldOk("rec3", DBR_DOUBLE, 32.0);

    testDiag("Wait for event");
    testOk1(mon.wait(3.0));
    testDiag("event");
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

#ifdef HAVE_UTAG
    testTrue(mon.changed.get(mon.root->getSubFieldT("fld1.timeStamp.userTag")->getFieldOffset()));
    mon.changed.clear(mon.root->getSubFieldT("fld1.timeStamp.userTag")->getFieldOffset());
#else
    testSkip(1, "!HAVE_UTAG");
#endif
    testEqual(mon.changed, pvd::BitSet()
              .set(mon.root->getSubFieldT("fld1.value")->getFieldOffset())
              .set(mon.root->getSubFieldT("fld1.alarm.severity")->getFieldOffset())
              .set(mon.root->getSubFieldT("fld1.alarm.status")->getFieldOffset())
              .set(mon.root->getSubFieldT("fld1.alarm.message")->getFieldOffset())
              .set(mon.root->getSubFieldT("fld1.timeStamp.secondsPastEpoch")->getFieldOffset())
              .set(mon.root->getSubFieldT("fld1.timeStamp.nanoseconds")->getFieldOffset()));

    testFieldEqual<pvd::PVDouble>(mon.root, "fld1.value", 32.0);
#else
    testSkip(21, "No multilock");
#endif
}

void testGroupMonitorTriggers(pvac::ClientProvider& client)
{
    testDiag("test group monitor w/ triggers");
#ifdef USE_MULTILOCK

    testdbPutFieldOk("rec5", DBR_DOUBLE, 5.0);
    testdbPutFieldOk("rec6", DBR_DOUBLE, 6.0);
    testdbPutFieldOk("rec5.RVAL", DBR_LONG, 50);

    testDiag("subscribe to grp2");
    pvac::MonitorSync mon(client.connect("grp2").monitor());

    testDiag("Wait for initial event");
    testOk1(mon.wait(3.0));
    testDiag("Initial event");
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

    testFieldEqual<pvd::PVDouble>(mon.root, "fld1.value", 5.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld2.value", 6.0);
    testFieldEqual<pvd::PVInt>(mon.root,    "fld3.value", 0); // not triggered -> no update.  only get/set

    testOk1(!mon.poll());

    testdbPutFieldOk("rec5.RVAL", DBR_LONG, 60); // no trigger -> no event
    testdbPutFieldOk("rec5", DBR_DOUBLE, 15.0); // no trigger -> no event
    testdbPutFieldOk("rec6", DBR_DOUBLE, 16.0); // event triggered

    testDiag("Wait for event");
    testOk1(mon.wait(3.0));
    testDiag("event");
    testOk1(mon.event.event==pvac::MonitorEvent::Data);
    if(!mon.poll())
        testAbort("Data event w/o data");

    testShow()<<mon.root;
#define OFF(NAME) mon.root->getSubFieldT(NAME)->getFieldOffset()
#ifdef HAVE_UTAG
    testTrue(mon.changed.get(OFF("fld1.timeStamp.userTag")));
    mon.changed.clear(OFF("fld1.timeStamp.userTag"));
    testTrue(mon.changed.get(OFF("fld2.timeStamp.userTag")));
    mon.changed.clear(OFF("fld2.timeStamp.userTag"));
#else
    testSkip(2, "!HAVE_UTAG");
#endif
    testEqual(mon.changed, pvd::BitSet()
              .set(OFF("fld1.value"))
              .set(OFF("fld1.alarm.severity"))
              .set(OFF("fld1.alarm.status"))
              .set(OFF("fld1.alarm.message"))
              .set(OFF("fld1.timeStamp.secondsPastEpoch"))
              .set(OFF("fld1.timeStamp.nanoseconds"))
              .set(OFF("fld2.value"))
              .set(OFF("fld2.alarm.severity"))
              .set(OFF("fld2.alarm.status"))
              .set(OFF("fld2.alarm.message"))
              .set(OFF("fld2.timeStamp.secondsPastEpoch"))
              .set(OFF("fld2.timeStamp.nanoseconds"))
              );
#undef OFF

    testFieldEqual<pvd::PVDouble>(mon.root, "fld1.value", 15.0);
    testFieldEqual<pvd::PVDouble>(mon.root, "fld2.value", 16.0);
    testFieldEqual<pvd::PVInt>(mon.root,    "fld3.value", 0); // not triggered -> no update.  only get/set

    testOk1(!mon.poll());
#else
    testSkip(21, "No multilock");
#endif
}

void testFilters(pvac::ClientProvider& client)
{
    testDiag("test w/ server side filters");

    pvd::shared_vector<const pvd::int16> expected;
    {
        pvd::shared_vector<pvd::int16> scratch(9);
        scratch[0] = 9;
        scratch[1] = 8;
        scratch[2] = 7;
        scratch[3] = 6;
        scratch[4] = 5;
        scratch[5] = 4;
        scratch[6] = 3;
        scratch[7] = 2;
        scratch[8] = 1;
        expected = pvd::freeze(scratch);
    }

    client.connect("TEST").put().set("value", expected).exec();

    pvd::PVStructure::const_shared_pointer root(client.connect("TEST").get());

    testFieldEqual<pvd::PVShortArray>(root, "value", expected);

    root = client.connect("TEST.{\"arr\":{\"i\":2}}").get();
    {
        pvd::shared_vector<pvd::int16> scratch(5);
        scratch[0] = 9;
        scratch[1] = 7;
        scratch[2] = 5;
        scratch[3] = 3;
        scratch[4] = 1;
        expected = pvd::freeze(scratch);
    }

    testFieldEqual<pvd::PVShortArray>(root, "value", expected);
}

} // namespace

extern "C"
void p2pTestIoc_registerRecordDeviceDriver(struct dbBase *);

MAIN(testpdb)
{
    testPlan(99);
    try{
        QSRVRegistrar_counters();
        epics::RefSnapshot ref_before;
        ref_before.update();

        testDiag("Refs before");
        for(epics::RefSnapshot::iterator it(ref_before.begin()), end(ref_before.end()); it!=end; ++it) {
            testDiag("Cnt %s = %zu (%ld)", it->first.c_str(), it->second.current, it->second.delta);
        }

        TestIOC IOC;

        testdbReadDatabase("p2pTestIoc.dbd", NULL, NULL);
        p2pTestIoc_registerRecordDeviceDriver(pdbbase);
        testdbReadDatabase("testpdb.db", NULL, NULL);
#ifdef USE_MULTILOCK
        testdbReadDatabase("testpdb-groups.db", NULL, NULL);
#endif
        testdbReadDatabase("testfilters.db", NULL, NULL);

        IOC.init();

        PDBProvider::shared_pointer prov(new PDBProvider());
        {
            pvac::ClientProvider client(prov);
            testSingleGet(client);
            testGroupGet(client);

            testSinglePut(client);
            testGroupPut(client);

            testSingleMonitor(client);
            testGroupMonitor(client);
            testGroupMonitorTriggers(client);
            testFilters(client);

            testEqual(epics::atomic::get(PDBProvider::num_instances), 1u);
        }

#ifndef USE_MULTILOCK
        // test w/ 3.15 leaves ref pvac::Monitor::Impl.  Probably transient.
        testTodoBegin("buggy test?");
#endif

        testOk1(prov.unique());
        prov.reset();

        testDiag("Refs after");
        epics::RefSnapshot ref_after;
        ref_after.update();
        epics::RefSnapshot ref_diff = ref_after - ref_before;
        for(epics::RefSnapshot::iterator it(ref_diff.begin()), end(ref_diff.end()); it!=end; ++it) {
            testDiag("Cnt %s = %zu (%ld)", it->first.c_str(), it->second.current, it->second.delta);
        }

        testDiag("check to see that all dbChannel are closed before IOC shuts down");
        testEqual(epics::atomic::get(PDBProvider::num_instances), 0u);
#ifdef USE_MULTILOCK
        testEqual(epics::atomic::get(PDBGroupChannel::num_instances), 0u);
        testEqual(epics::atomic::get(PDBGroupPV::num_instances), 0u);
#else
        testSkip(2, "No multilock");
#endif // USE_MULTILOCK
        testEqual(epics::atomic::get(PDBSinglePV::num_instances), 0u);

        testTodoEnd();

    }catch(std::exception& e){
        PRINT_EXCEPTION(e);
        testAbort("Unexpected Exception: %s", e.what());
    }
    return testDone();
}
