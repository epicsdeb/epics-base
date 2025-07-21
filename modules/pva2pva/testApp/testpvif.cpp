
#include <testMain.h>

#include <iocsh.h>
#include <dbAccess.h>
#include <longinRecord.h>
#include <aiRecord.h>
#include <mbbiRecord.h>
#include <stringinRecord.h>
#include <epicsVersion.h>

#ifdef EPICS_VERSION_INT
#  if EPICS_VERSION_INT>=VERSION_INT(3,16,1,0)
#    define USE_INT64
#    include <int64inRecord.h>
#  endif
#endif

#include "helper.h"
#include "pvif.h"
#include "utilities.h"

namespace pvd = epics::pvData;

extern "C"
void p2pTestIoc_registerRecordDeviceDriver(struct dbBase *);

namespace {

void testScalar()
{
    testDiag("======= testScalar() ======");

    TestIOC IOC;

    testdbReadDatabase("p2pTestIoc.dbd", NULL, NULL);
    p2pTestIoc_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("testpvif.db", NULL, NULL);
#ifdef USE_INT64
    testdbReadDatabase("testpvif64.db", NULL, NULL);
#endif

    longinRecord *prec_li = (longinRecord*)testdbRecordPtr("test:li");
    stringinRecord *prec_si = (stringinRecord*)testdbRecordPtr("test:si");
    aiRecord *prec_ai = (aiRecord*)testdbRecordPtr("test:ai");
    mbbiRecord *prec_mbbi = (mbbiRecord*)testdbRecordPtr("test:mbbi");
#ifdef USE_INT64
    int64inRecord *prec_i64 = (int64inRecord*)testdbRecordPtr("test:i64");
#endif

    IOC.init();

    testdbGetFieldEqual("test:mbbi", DBR_STRING, "one");
    testdbGetFieldEqual("test:mbbi", DBR_SHORT, 1);

    DBCH chan_li("test:li");
    DBCH chan_si("test:si");
    DBCH chan_ai("test:ai");
    DBCH chan_ai_rval("test:ai.RVAL");
    DBCH chan_mbbi("test:mbbi");
#ifdef USE_INT64
    DBCH chan_i64("test:i64");
#endif
    testEqual(dbChannelFieldType(chan_li), DBR_LONG);
    testEqual(dbChannelFieldType(chan_si), DBR_STRING);
    testEqual(dbChannelFieldType(chan_ai), DBR_DOUBLE);
    testEqual(dbChannelFieldType(chan_ai_rval), DBR_LONG);
    testEqual(dbChannelFieldType(chan_mbbi), DBR_ENUM);
    testEqual(dbChannelFinalFieldType(chan_li), DBR_LONG);
    testEqual(dbChannelFinalFieldType(chan_ai), DBR_DOUBLE);
    testEqual(dbChannelFinalFieldType(chan_ai_rval), DBR_LONG);
    testEqual(dbChannelFinalFieldType(chan_mbbi), DBR_ENUM);
#ifdef USE_INT64
    testEqual(dbChannelFinalFieldType(chan_i64), DBR_INT64);
#else
    testSkip(1, "!USE_INT64");
#endif

    pvd::PVStructurePtr root;
    p2p::auto_ptr<PVIF> pvif_li;
#ifdef USE_INT64
    p2p::auto_ptr<PVIF> pvif_i64;
#endif
    p2p::auto_ptr<PVIF> pvif_si;
    p2p::auto_ptr<PVIF> pvif_ai;
    p2p::auto_ptr<PVIF> pvif_ai_rval;
    p2p::auto_ptr<PVIF> pvif_mbbi;
    {
        ScalarBuilder builder_li(chan_li);
#ifdef USE_INT64
        ScalarBuilder builder_i64(chan_i64);
#endif
        ScalarBuilder builder_si(chan_si);
        ScalarBuilder builder_ai(chan_ai);
        ScalarBuilder builder_ai_rval(chan_ai_rval);
        ScalarBuilder builder_mbbi(chan_mbbi);

        pvd::FieldConstPtr dtype_li(builder_li.dtype());
    #ifdef USE_INT64
        pvd::FieldConstPtr dtype_i64(builder_i64.dtype());
    #endif
        pvd::FieldConstPtr dtype_si(builder_si.dtype());
        pvd::FieldConstPtr dtype_ai(builder_ai.dtype());
        pvd::FieldConstPtr dtype_ai_rval(builder_ai_rval.dtype());
        pvd::FieldConstPtr dtype_mbbi(builder_mbbi.dtype());

        pvd::StructureConstPtr dtype_root(pvd::getFieldCreate()->createFieldBuilder()
                                          ->add("li", dtype_li)
    #ifdef USE_INT64
                                          ->add("i64", dtype_i64)
    #endif
                                          ->add("si", dtype_si)
                                          ->add("ai", dtype_ai)
                                          ->add("ai_rval", dtype_ai_rval)
                                          ->add("mbbi", dtype_mbbi)
                                          ->createStructure());

        root = pvd::getPVDataCreate()->createPVStructure(dtype_root);

        pvif_li.reset(builder_li.attach(root, FieldName("li")));
    #ifdef USE_INT64
        pvif_i64.reset(builder_i64.attach(root, FieldName("i64")));
    #endif
        pvif_si.reset(builder_si.attach(root, FieldName("si")));
        pvif_ai.reset(builder_ai.attach(root, FieldName("ai")));
        pvif_ai_rval.reset(builder_ai_rval.attach(root, FieldName("ai_rval")));
        pvif_mbbi.reset(builder_mbbi.attach(root, FieldName("mbbi")));
    }

    testShow()<<"Entire structure\n"<<root;

    pvd::BitSet mask;

    dbScanLock((dbCommon*)prec_li);
    prec_li->time.secPastEpoch = 0x12345678;
    prec_li->time.nsec = 12345678;
    pvif_li->put(mask, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, NULL);
    dbScanUnlock((dbCommon*)prec_li);

#define OFF(NAME) (epicsUInt32)root->getSubFieldT(NAME)->getFieldOffset()
#ifdef HAVE_UTAG
    testTrue(mask.get(OFF("li.timeStamp.userTag")));
    mask.clear(OFF("li.timeStamp.userTag"));
#else
    testSkip(1, "!HAVE_UTAG");
#endif
    testEqual(mask, pvd::BitSet()
              .set(OFF("li.value"))
              .set(OFF("li.alarm.severity"))
              .set(OFF("li.alarm.status"))
              .set(OFF("li.alarm.message"))
              .set(OFF("li.timeStamp.secondsPastEpoch"))
              .set(OFF("li.timeStamp.nanoseconds"))
              .set(OFF("li.display.limitHigh"))
              .set(OFF("li.display.limitLow"))
              .set(OFF("li.display.description"))
              .set(OFF("li.display.units"))
              .set(OFF("li.display.precision"))
              .set(OFF("li.display.form.index"))
              .set(OFF("li.control.limitHigh"))
              .set(OFF("li.control.limitLow"))
              .set(OFF("li.valueAlarm.highWarningLimit"))
              .set(OFF("li.valueAlarm.lowWarningLimit"))
              .set(OFF("li.valueAlarm.highAlarmLimit"))
              .set(OFF("li.valueAlarm.lowAlarmLimit")))
            <<" li changes\n"<<root->stream().show(mask);
#undef OFF
    mask.clear();

#ifdef USE_INT64

    dbScanLock((dbCommon*)prec_i64);
    prec_i64->time.secPastEpoch = 0x12345678;
    prec_i64->time.nsec = 12345678;
    pvif_i64->put(mask, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, NULL);
    dbScanUnlock((dbCommon*)prec_i64);

#define OFF(NAME) (epicsUInt32)root->getSubFieldT(NAME)->getFieldOffset()
#ifdef HAVE_UTAG
    testTrue(mask.get(OFF("i64.timeStamp.userTag")));
    mask.clear(OFF("i64.timeStamp.userTag"));
#else
    testSkip(1, "!HAVE_UTAG");
#endif
    testEqual(mask, pvd::BitSet()
              .set(OFF("i64.value"))
              .set(OFF("i64.alarm.severity"))
              .set(OFF("i64.alarm.status"))
              .set(OFF("i64.alarm.message"))
              .set(OFF("i64.timeStamp.secondsPastEpoch"))
              .set(OFF("i64.timeStamp.nanoseconds"))
              .set(OFF("i64.display.limitHigh"))
              .set(OFF("i64.display.limitLow"))
              .set(OFF("i64.display.description"))
              .set(OFF("i64.display.units"))
              .set(OFF("i64.display.precision"))
              .set(OFF("i64.display.form.index"))
              .set(OFF("i64.control.limitHigh"))
              .set(OFF("i64.control.limitLow"))
              .set(OFF("i64.valueAlarm.highWarningLimit"))
              .set(OFF("i64.valueAlarm.lowWarningLimit"))
              .set(OFF("i64.valueAlarm.highAlarmLimit"))
              .set(OFF("i64.valueAlarm.lowAlarmLimit")))
            <<" i64 changes\n"<<root->stream().show(mask);
#undef OFF
    mask.clear();

#else // !USE_INT64
    testSkip(2, "!USE_INT64");

#endif // USE_INT64

    dbScanLock((dbCommon*)prec_si);
    prec_si->time.secPastEpoch = 0x12345678;
    prec_si->time.nsec = 12345678;
    pvif_si->put(mask, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, NULL);
    dbScanUnlock((dbCommon*)prec_si);

#define OFF(NAME) (epicsUInt32)root->getSubFieldT(NAME)->getFieldOffset()
#ifdef HAVE_UTAG
    testTrue(mask.get(OFF("si.timeStamp.userTag")));
    mask.clear(OFF("si.timeStamp.userTag"));
#else
    testSkip(1, "!HAVE_UTAG");
#endif
    testEqual(mask, pvd::BitSet()
              .set(OFF("si.value"))
              .set(OFF("si.alarm.severity"))
              .set(OFF("si.alarm.status"))
              .set(OFF("si.alarm.message"))
              .set(OFF("si.timeStamp.secondsPastEpoch"))
              .set(OFF("si.timeStamp.nanoseconds"))
              .set(OFF("si.display.limitHigh"))
              .set(OFF("si.display.limitLow"))
              .set(OFF("si.display.description"))
              .set(OFF("si.display.units"))
              .set(OFF("si.display.precision"))
              .set(OFF("si.display.form.index"))
              .set(OFF("si.control.limitHigh"))
              .set(OFF("si.control.limitLow")))
            <<" si changes\n"<<root->stream().show(mask);
#undef OFF
    mask.clear();

    dbScanLock((dbCommon*)prec_ai);
    prec_ai->time.secPastEpoch = 0x12345678;
    prec_ai->time.nsec = 12345678;
    pvif_ai->put(mask, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, NULL);
    pvif_ai_rval->put(mask, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, NULL);
    dbScanUnlock((dbCommon*)prec_ai);

#define OFF(NAME) (epicsUInt32)root->getSubFieldT(NAME)->getFieldOffset()
#ifdef HAVE_UTAG
    testTrue(mask.get(OFF("ai.timeStamp.userTag")));
    mask.clear(OFF("ai.timeStamp.userTag"));
    testTrue(mask.get(OFF("ai_rval.timeStamp.userTag")));
    mask.clear(OFF("ai_rval.timeStamp.userTag"));
#else
    testSkip(2, "!HAVE_UTAG");
#endif
    testEqual(mask, pvd::BitSet()
              .set(OFF("ai.value"))
              .set(OFF("ai.alarm.severity"))
              .set(OFF("ai.alarm.status"))
              .set(OFF("ai.alarm.message"))
              .set(OFF("ai.timeStamp.secondsPastEpoch"))
              .set(OFF("ai.timeStamp.nanoseconds"))
              .set(OFF("ai.display.limitHigh"))
              .set(OFF("ai.display.limitLow"))
              .set(OFF("ai.display.description"))
              .set(OFF("ai.display.precision"))
              .set(OFF("ai.display.form.index"))
              .set(OFF("ai.display.units"))
              .set(OFF("ai.control.limitHigh"))
              .set(OFF("ai.control.limitLow"))
              .set(OFF("ai.valueAlarm.highWarningLimit"))
              .set(OFF("ai.valueAlarm.lowWarningLimit"))
              .set(OFF("ai.valueAlarm.highAlarmLimit"))
              .set(OFF("ai.valueAlarm.lowAlarmLimit"))
              .set(OFF("ai_rval.value"))
              .set(OFF("ai_rval.alarm.severity"))
              .set(OFF("ai_rval.alarm.status"))
              .set(OFF("ai_rval.alarm.message"))
              .set(OFF("ai_rval.timeStamp.secondsPastEpoch"))
              .set(OFF("ai_rval.timeStamp.nanoseconds"))
              .set(OFF("ai_rval.display.limitHigh"))
              .set(OFF("ai_rval.display.limitLow"))
              .set(OFF("ai_rval.display.description"))
              .set(OFF("ai_rval.display.units"))
              .set(OFF("ai_rval.display.precision"))
              .set(OFF("ai_rval.display.form.index"))
              .set(OFF("ai_rval.control.limitHigh"))
              .set(OFF("ai_rval.control.limitLow"))
              .set(OFF("ai_rval.valueAlarm.highWarningLimit"))
              .set(OFF("ai_rval.valueAlarm.lowWarningLimit"))
              .set(OFF("ai_rval.valueAlarm.highAlarmLimit"))
              .set(OFF("ai_rval.valueAlarm.lowAlarmLimit"))
              )
            <<" ai changes\n"<<root->stream().show(mask);
#undef OFF
    mask.clear();

    dbScanLock((dbCommon*)prec_mbbi);
    prec_mbbi->time.secPastEpoch = 0x12345678;
    prec_mbbi->time.nsec = 0x12345678;
    pvif_mbbi->put(mask, DBE_VALUE|DBE_ALARM|DBE_PROPERTY, NULL);
    dbScanUnlock((dbCommon*)prec_mbbi);

#define OFF(NAME) root->getSubFieldT(NAME)->getFieldOffset()
    testEqual(mask, pvd::BitSet()
              .set(OFF("mbbi.value.index"))
              .set(OFF("mbbi.value.choices"))
              .set(OFF("mbbi.alarm.severity"))
              .set(OFF("mbbi.alarm.status"))
              .set(OFF("mbbi.alarm.message"))
              .set(OFF("mbbi.timeStamp.secondsPastEpoch"))
              .set(OFF("mbbi.timeStamp.nanoseconds"))
              .set(OFF("mbbi.timeStamp.userTag")))
            <<" mbbi changes\n"<<root->stream().show(mask);
#undef OFF
    mask.clear();

    testFieldEqual<pvd::PVInt>(root, "li.value", 102042);
    testFieldEqual<pvd::PVInt>(root, "li.alarm.severity", 1);
    testFieldEqual<pvd::PVInt>(root, "li.alarm.status", 1);
    testFieldEqual<pvd::PVLong>(root, "li.timeStamp.secondsPastEpoch", 0x12345678+POSIX_TIME_AT_EPICS_EPOCH);
    testFieldEqual<pvd::PVInt>(root, "li.timeStamp.nanoseconds", 12345678);
    testFieldEqual<pvd::PVDouble>(root, "li.display.limitHigh", 100.0);
    testFieldEqual<pvd::PVDouble>(root, "li.display.limitLow", 10.0);
    testFieldEqual<pvd::PVString>(root, "li.display.units", "arb");
    testFieldEqual<pvd::PVInt>(root, "li.display.precision", 0);
    testFieldEqual<pvd::PVInt>(root, "li.display.form.index", 4); // "Hex"

#ifdef USE_INT64
    testFieldEqual<pvd::PVLong>(root, "i64.value", 0x7fffffffffffffffLL);
    testFieldEqual<pvd::PVInt>(root, "i64.alarm.severity", 1);
    testFieldEqual<pvd::PVInt>(root, "i64.alarm.status", 1);
    testFieldEqual<pvd::PVLong>(root, "i64.timeStamp.secondsPastEpoch", 0x12345678+POSIX_TIME_AT_EPICS_EPOCH);
    testFieldEqual<pvd::PVInt>(root, "i64.timeStamp.nanoseconds", 12345678);
    testFieldEqual<pvd::PVDouble>(root, "i64.display.limitHigh", 100.0);
    testFieldEqual<pvd::PVDouble>(root, "i64.display.limitLow", 10.0);
    testTodoBegin("Bug in int64inRecord get_units()");
    testFieldEqual<pvd::PVString>(root, "i64.display.units", "arb");
    testTodoEnd();
    testFieldEqual<pvd::PVInt>(root, "i64.display.precision", 0);
#else
    testSkip(9, "!USE_INT64");
#endif

    testFieldEqual<pvd::PVString>(root, "si.value", "hello");
    testFieldEqual<pvd::PVInt>(root, "si.alarm.severity", 0);
    testFieldEqual<pvd::PVLong>(root, "si.timeStamp.secondsPastEpoch", 0x12345678+POSIX_TIME_AT_EPICS_EPOCH);
    testFieldEqual<pvd::PVInt>(root, "si.timeStamp.nanoseconds", 12345678);

    testFieldEqual<pvd::PVDouble>(root, "ai.value", 42.2);
    testFieldEqual<pvd::PVInt>(root, "ai.alarm.severity", 2);
    testFieldEqual<pvd::PVInt>(root, "ai.alarm.status", 1);
    testFieldEqual<pvd::PVLong>(root, "ai.timeStamp.secondsPastEpoch", 0x12345678+POSIX_TIME_AT_EPICS_EPOCH);
    testFieldEqual<pvd::PVInt>(root, "ai.timeStamp.nanoseconds", 12345678);
    testFieldEqual<pvd::PVDouble>(root, "ai.display.limitHigh", 200.0);
    testFieldEqual<pvd::PVDouble>(root, "ai.display.limitLow", 20.0);
    testFieldEqual<pvd::PVInt>(root, "ai.display.precision", 2);
    testFieldEqual<pvd::PVString>(root, "ai.display.units", "foo");
    testFieldEqual<pvd::PVInt>(root, "ai.display.form.index", 0);

    testFieldEqual<pvd::PVInt>(root, "ai_rval.value", 1234);
    testFieldEqual<pvd::PVInt>(root, "ai_rval.alarm.severity", 2);
    testFieldEqual<pvd::PVLong>(root, "ai_rval.timeStamp.secondsPastEpoch", 0x12345678+POSIX_TIME_AT_EPICS_EPOCH);
    testFieldEqual<pvd::PVInt>(root, "ai_rval.timeStamp.nanoseconds", 12345678);
    testFieldEqual<pvd::PVInt>(root, "ai_rval.timeStamp.userTag", 0);
    testFieldEqual<pvd::PVDouble>(root, "ai_rval.display.limitHigh", 2147483647.0);
    testFieldEqual<pvd::PVDouble>(root, "ai_rval.display.limitLow", -2147483648.0);
    testFieldEqual<pvd::PVInt>(root, "ai_rval.display.precision", 0);
    testFieldEqual<pvd::PVString>(root, "ai_rval.display.units", "");
    testFieldEqual<pvd::PVInt>(root, "ai_rval.display.form.index", 0);

    testFieldEqual<pvd::PVInt>(root, "mbbi.value.index", 1);
    testFieldEqual<pvd::PVInt>(root, "mbbi.alarm.severity", 0);
    testFieldEqual<pvd::PVLong>(root, "mbbi.timeStamp.secondsPastEpoch", 0x12345678+POSIX_TIME_AT_EPICS_EPOCH);
    testFieldEqual<pvd::PVInt>(root, "mbbi.timeStamp.nanoseconds", 0x12345670);
    testFieldEqual<pvd::PVInt>(root, "mbbi.timeStamp.userTag", 0x8);
    {
        pvd::PVStringArray::const_svector choices(root->getSubFieldT<pvd::PVStringArray>("mbbi.value.choices")->view());
        testOk1(choices.size()==3);
        testOk1(choices.size()>0 && choices[0]=="zero");
        testOk1(choices.size()>1 && choices[1]=="one");
        testOk1(choices.size()>2 && choices[2]=="two");
    }

    root->getSubFieldT<pvd::PVInt>("li.value")->put(102043);
#ifdef USE_INT64
    root->getSubFieldT<pvd::PVLong>("i64.value")->put(-0x8000000000000000LL);
#endif
    root->getSubFieldT<pvd::PVString>("si.value")->put("world");
    root->getSubFieldT<pvd::PVDouble>("ai.value")->put(44.4);
    root->getSubFieldT<pvd::PVInt>("ai_rval.value")->put(2143);
    root->getSubFieldT<pvd::PVInt>("mbbi.value.index")->put(2);

    dbScanLock((dbCommon*)prec_li);
    mask.clear();
    mask.set(root->getSubFieldT("li.value")->getFieldOffset());
    pvif_li->get(mask);
    testEqual(prec_li->val, 102043);
    dbScanUnlock((dbCommon*)prec_li);

#ifdef USE_INT64
    dbScanLock((dbCommon*)prec_i64);
    mask.clear();
    mask.set(root->getSubFieldT("i64.value")->getFieldOffset());
    pvif_i64->get(mask);
    testEqual(prec_i64->val, epicsInt64(-0x8000000000000000LL));
    dbScanUnlock((dbCommon*)prec_i64);

    dbScanLock((dbCommon*)prec_i64);
    mask.clear();
    mask.set(root->getSubFieldT("i64.value")->getFieldOffset());
    pvif_i64->get(mask);
    testEqual(prec_i64->val, epicsInt64(-0x8000000000000000LL));
    dbScanUnlock((dbCommon*)prec_i64);
#else
    testSkip(2, "!USE_INT64");
#endif

    dbScanLock((dbCommon*)prec_si);
    mask.clear();
    mask.set(root->getSubFieldT("si.value")->getFieldOffset());
    pvif_si->get(mask);
    testOk(strcmp(prec_si->val, "world")==0, "\"%s\" == \"%s\"", prec_si->val, "world");
    dbScanUnlock((dbCommon*)prec_si);

    dbScanLock((dbCommon*)prec_ai);
    mask.clear();
    mask.set(root->getSubFieldT("ai.value")->getFieldOffset());
    pvif_ai->get(mask);
    testEqual(prec_ai->val, 44.4);
    dbScanUnlock((dbCommon*)prec_ai);

    dbScanLock((dbCommon*)prec_ai);
    mask.clear();
    mask.set(root->getSubFieldT("ai_rval.value")->getFieldOffset());
    pvif_ai_rval->get(mask);
    testEqual(prec_ai->rval, 2143);
    dbScanUnlock((dbCommon*)prec_ai);

    dbScanLock((dbCommon*)prec_mbbi);
    mask.clear();
    mask.set(root->getSubFieldT("mbbi.value.index")->getFieldOffset());
    pvif_mbbi->get(mask);
    testEqual(prec_mbbi->val, 2);
    dbScanUnlock((dbCommon*)prec_mbbi);
}

void testPlain()
{
    testDiag("testPlain()");

    TestIOC IOC;

    testdbReadDatabase("p2pTestIoc.dbd", NULL, NULL);
    p2pTestIoc_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("testpvif.db", NULL, NULL);

    longinRecord *prec_li = (longinRecord*)testdbRecordPtr("test:li");
    stringinRecord *prec_si = (stringinRecord*)testdbRecordPtr("test:si");
    aiRecord *prec_ai = (aiRecord*)testdbRecordPtr("test:ai");
    mbbiRecord *prec_mbbi = (mbbiRecord*)testdbRecordPtr("test:mbbi");

    IOC.init();

    DBCH chan_li("test:li");
    DBCH chan_si("test:si");
    DBCH chan_ai("test:ai");
    DBCH chan_mbbi("test:mbbi");

    pvd::PVStructurePtr root;
    p2p::auto_ptr<PVIF> pvif_li;
    p2p::auto_ptr<PVIF> pvif_si;
    p2p::auto_ptr<PVIF> pvif_ai;
    p2p::auto_ptr<PVIF> pvif_mbbi;
    {
        p2p::auto_ptr<PVIFBuilder> builder_li(PVIFBuilder::create("plain", chan_li));
        p2p::auto_ptr<PVIFBuilder> builder_si(PVIFBuilder::create("plain", chan_si));
        p2p::auto_ptr<PVIFBuilder> builder_ai(PVIFBuilder::create("plain", chan_ai));
        p2p::auto_ptr<PVIFBuilder> builder_mbbi(PVIFBuilder::create("plain", chan_mbbi));

        pvd::FieldConstPtr dtype_li(builder_li->dtype());
        pvd::FieldConstPtr dtype_si(builder_si->dtype());
        pvd::FieldConstPtr dtype_ai(builder_ai->dtype());
        pvd::FieldConstPtr dtype_mbbi(builder_mbbi->dtype());

        pvd::StructureConstPtr dtype_root(pvd::getFieldCreate()->createFieldBuilder()
                                          ->add("li", dtype_li)
                                          ->add("si", dtype_si)
                                          ->add("ai", dtype_ai)
                                          ->add("mbbi", dtype_mbbi)
                                          ->createStructure());

        root = pvd::getPVDataCreate()->createPVStructure(dtype_root);

        pvif_li.reset(builder_li->attach(root, FieldName("li")));
        pvif_si.reset(builder_si->attach(root, FieldName("si")));
        pvif_ai.reset(builder_ai->attach(root, FieldName("ai")));
        pvif_mbbi.reset(builder_mbbi->attach(root, FieldName("mbbi")));
    }

    pvd::BitSet mask;

    mask.clear();
    dbScanLock((dbCommon*)prec_li);
    pvif_li->put(mask, DBE_VALUE, NULL);
    dbScanUnlock((dbCommon*)prec_li);

    testEqual(mask, pvd::BitSet().set(root->getSubFieldT("li")->getFieldOffset()));

    testFieldEqual<pvd::PVInt>(root, "li", 102042);

    mask.clear();
    dbScanLock((dbCommon*)prec_si);
    pvif_si->put(mask, DBE_VALUE, NULL);
    dbScanUnlock((dbCommon*)prec_si);

    testEqual(mask, pvd::BitSet().set(root->getSubFieldT("si")->getFieldOffset()));

    testFieldEqual<pvd::PVString>(root, "si", "hello");

    mask.clear();
    dbScanLock((dbCommon*)prec_ai);
    pvif_ai->put(mask, DBE_VALUE, NULL);
    dbScanUnlock((dbCommon*)prec_ai);

    testEqual(mask, pvd::BitSet().set(root->getSubFieldT("ai")->getFieldOffset()));

    testFieldEqual<pvd::PVDouble>(root, "ai", 42.2);

    mask.clear();
    dbScanLock((dbCommon*)prec_mbbi);
    pvif_mbbi->put(mask, DBE_VALUE, NULL);
    dbScanUnlock((dbCommon*)prec_mbbi);

    testEqual(mask, pvd::BitSet().set(root->getSubFieldT("mbbi")->getFieldOffset()));

    testFieldEqual<pvd::PVInt>(root, "mbbi", 1);


    root->getSubFieldT<pvd::PVInt>("li")->put(102043);
    root->getSubFieldT<pvd::PVString>("si")->put("world");
    root->getSubFieldT<pvd::PVDouble>("ai")->put(44.4);
    root->getSubFieldT<pvd::PVInt>("mbbi")->put(2);

    dbScanLock((dbCommon*)prec_li);
    mask.clear();
    mask.set(root->getSubFieldT("li")->getFieldOffset());
    pvif_li->get(mask);
    testEqual(prec_li->val, 102043);
    dbScanUnlock((dbCommon*)prec_li);

    dbScanLock((dbCommon*)prec_si);
    mask.clear();
    mask.set(root->getSubFieldT("si")->getFieldOffset());
    pvif_si->get(mask);
    testOk(strcmp(prec_si->val, "world")==0, "\"%s\" == \"%s\"", prec_si->val, "world");
    dbScanUnlock((dbCommon*)prec_si);

    dbScanLock((dbCommon*)prec_ai);
    mask.clear();
    mask.set(root->getSubFieldT("ai")->getFieldOffset());
    pvif_ai->get(mask);
    testEqual(prec_ai->val, 44.4);
    dbScanUnlock((dbCommon*)prec_ai);

    dbScanLock((dbCommon*)prec_mbbi);
    mask.clear();
    mask.set(root->getSubFieldT("mbbi")->getFieldOffset());
    pvif_mbbi->get(mask);
    testEqual(prec_mbbi->val, 2);
    dbScanUnlock((dbCommon*)prec_mbbi);
}

void testFilters()
{
    testDiag("testFilter");

#if EPICS_VERSION_INT < VERSION_INT(7, 0, 0, 0)
    testSkip(5, "Needs Base >=7.0");
#else

    TestIOC IOC;

    testdbReadDatabase("p2pTestIoc.dbd", NULL, NULL);
    p2pTestIoc_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("testfilters.db", NULL, NULL);

    IOC.init();

    static const epicsInt32 arr[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    testdbPutArrFieldOk("TEST", DBF_LONG, 9, arr);

#if EPICS_VERSION_INT > VERSION_INT(7, 0, 5, 0)
    testdbGetArrFieldEqual("TEST", DBF_LONG, 10, 9, arr);
    testdbGetArrFieldEqual("TEST.{\"arr\":{\"s\":5}}", DBF_LONG, 10, 4, arr+5);

    static const epicsInt32 arr2[] = {9, 7, 5, 3, 1};
    testdbGetArrFieldEqual("TEST.{\"arr\":{\"i\":2}}", DBF_LONG, 10, 5, arr2);

#else
    testSkip(3, "dbUnitTest doesn't use dbChannel");
#endif

    pvd::PVStructurePtr root;
    p2p::auto_ptr<PVIF> pvif;

    DBCH chan("TEST.{\"arr\":{\"i\":2}}");
    ScalarBuilder builder(chan);

    root = pvd::FieldBuilder::begin()
            ->add("dut", builder.dtype())
            ->createStructure()->build();

    pvif.reset(builder.attach(root, FieldName("dut")));

    LocalFL fl(0, chan.chan);

    pvd::shared_vector<pvd::int16> scratch(5);
    scratch[0] = 9;
    scratch[1] = 7;
    scratch[2] = 5;
    scratch[3] = 3;
    scratch[4] = 1;
    pvd::shared_vector<const pvd::int16> expected(pvd::freeze(scratch));

    dbCommon *prec =  testdbRecordPtr("TEST");
    dbScanLock(prec);
    pvd::BitSet changed;
    pvif->put(changed, DBE_VALUE, fl.pfl);
    dbScanUnlock(prec);

    testFieldEqual<pvd::PVShortArray>(root, "dut.value", expected);
#endif // >= 7.0
}

} // namespace

MAIN(testpvif)
{
    testPlan(98);
#ifdef USE_INT64
    testDiag("Testing of 64-bit field access");
#else
    testDiag("64-bit field access not supported");
#endif
    testScalar();
    testPlain();
    testFilters();
    return testDone();
}
