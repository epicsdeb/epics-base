

#include <pv/pvIntrospect.h> /* for pvdVersion.h */
#include <pv/standardField.h>

#include <dbAccess.h>
#include <dbChannel.h>
#include <dbStaticLib.h>
#include <dbLock.h>
#include <dbEvent.h>
#include <alarm.h>
#include <errSymTbl.h>
#include <epicsVersion.h>
#include <errlog.h>
#include <osiSock.h>

#include <pv/status.h>
#include <pv/bitSet.h>
#include <pv/pvData.h>
#include <pv/anyscalar.h>
#include <pv/reftrack.h>
#include <pv/pvAccess.h>
#include <pv/security.h>

#include "sb.h"
#include "pvif.h"

#include <epicsExport.h>

#ifdef EPICS_VERSION_INT
#  if EPICS_VERSION_INT>=VERSION_INT(3,16,1,0)
#    define USE_INT64
     // effects all uses of pv/typemap.h
#    define CASE_REAL_INT64
#  endif
#endif

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

DBCH::DBCH(dbChannel *ch) :chan(ch)
{
    if(!chan)
        throw std::invalid_argument("NULL channel");
    prepare();
}

DBCH::DBCH(const std::string& name)
    :chan(dbChannelCreate(name.c_str()))
{
    if(!chan)
        throw std::invalid_argument(SB()<<"invalid channel: "<<name);
    prepare();
}

void DBCH::prepare()
{
    if(!chan)
        throw std::invalid_argument(SB()<<"NULL channel");
    if(dbChannelOpen(chan)) {
        dbChannelDelete(chan);
        throw std::invalid_argument(SB()<<"Failed to open channel "<<dbChannelName(chan));
    }
}

DBCH::~DBCH()
{
    if(chan) dbChannelDelete(chan);
}

void DBCH::swap(DBCH& o)
{
    std::swap(chan, o.chan);
}

void ASCred::update(const pva::ChannelRequester::shared_pointer& req)
{
    pva::PeerInfo::const_shared_pointer info(req->getPeerInfo());
    std::string usertemp, hosttemp;

    if(info && info->identified) {
        hosttemp = info->peer;
        if(info->authority=="ca") {
            usertemp = info->account;
            size_t sep = usertemp.find_last_of('/');
            if(sep != std::string::npos) {
                // prevent CA auth from claiming to be eg. "krb/someone.special"
                usertemp = usertemp.substr(sep+1);
            }

        } else {
            usertemp = info->authority + "/" + info->account;
        }

        const char role[] = "role/";

        groups.resize(info->roles.size());
        size_t idx = 0u;
        for(pva::PeerInfo::roles_t::const_iterator it(info->roles.begin()), end(info->roles.end()); it!=end; ++it, idx++) {
            groups[idx].resize((*it).size()+sizeof(role)); // sizeof(role) includes trailing nil
            std::copy(role,
                      role+sizeof(role)-1,
                      groups[idx].begin());
            std::copy(it->begin(),
                      it->end(),
                      groups[idx].begin()+sizeof(role)-1);
            groups[idx][groups[idx].size()-1] = '\0';
        }

    } else {
        // legacy and anonymous
        hosttemp = req->getRequesterName();
    }

    // remote names have the form "IP:port"
    size_t sep = hosttemp.find_first_of(':');
    if(sep == std::string::npos) {
        sep = hosttemp.size();
    }
    hosttemp.resize(sep);

    host.resize(hosttemp.size()+1);
    std::copy(hosttemp.begin(),
              hosttemp.end(),
              host.begin());
    host[hosttemp.size()] = '\0';

    user.resize(usertemp.size()+1);
    std::copy(usertemp.begin(),
              usertemp.end(),
              user.begin());
    user[usertemp.size()] = '\0';
}

ASCLIENT::~ASCLIENT()
{
    asRemoveClient(&aspvt);
    for(size_t i=0, N=grppvt.size(); i<N; i++) {
        asRemoveClient(&grppvt[i]);
    }
}

void ASCLIENT::add(dbChannel* chan, ASCred& cred)
{
    asRemoveClient(&aspvt);
    /* asAddClient() fails secure to no-permission */
    (void)asAddClient(&aspvt, dbChannelRecord(chan)->asp, dbChannelFldDes(chan)->as_level, &cred.user[0], &cred.host[0]);

    grppvt.resize(cred.groups.size(), 0);

    for(size_t i=0, N=grppvt.size(); i<N; i++) {
        asRemoveClient(&grppvt[i]);
        (void)asAddClient(&grppvt[i], dbChannelRecord(chan)->asp, dbChannelFldDes(chan)->as_level, &cred.groups[i][0], &cred.host[0]);
    }
}

bool ASCLIENT::canWrite() {
    if(!asActive || (aspvt && asCheckPut(aspvt)))
        return true;
    for(size_t i=0, N=grppvt.size(); i<N; i++) {
        if(grppvt[i] && asCheckPut(grppvt[i]))
            return true;
    }
    return false;
}

PVIF::PVIF(dbChannel *ch)
    :chan(ch)
{}

namespace {

struct pvTimeAlarm {
    dbChannel *chan;

    pvd::uint32 nsecMask;

    pvd::BitSet maskALWAYS, maskALARM;

    pvd::PVLongPtr sec;
    pvd::PVIntPtr status, severity, nsec, userTag;
    pvd::PVStringPtr message;

    pvTimeAlarm() :chan(NULL), nsecMask(0) {}
};

struct pvCommon : public pvTimeAlarm {

    pvd::BitSet maskVALUE, maskPROPERTY, maskVALUEPut;

    pvd::PVDoublePtr displayLow, displayHigh, controlLow, controlHigh;
    pvd::PVStringPtr egu, desc;
    pvd::PVIntPtr fmt, prec;

    pvd::PVScalarPtr warnLow, warnHigh, alarmLow, alarmHigh;

    pvd::PVStringArrayPtr enumopts;
};

struct pvScalar : public pvCommon {
    typedef pvd::PVScalar pvd_type;
    pvd::PVScalarPtr value;
};

struct pvArray : public pvCommon {
    typedef pvd::PVScalarArray pvd_type;
    pvd::PVScalarArrayPtr value;
};

struct metaTIME {
    DBRstatus
    DBRamsg
    DBRtime
    DBRutag

    enum {mask = DBR_STATUS | DBR_AMSG | DBR_TIME | DBR_UTAG};
};

struct metaDOUBLE {
    DBRstatus
    DBRamsg
    DBRunits
    DBRprecision
    DBRtime
    DBRutag
    DBRgrDouble
    DBRctrlDouble
    DBRalDouble

    // similar junk
    DBRenumStrs

    enum {mask = DBR_STATUS | DBR_AMSG | DBR_UNITS | DBR_PRECISION | DBR_TIME | DBR_UTAG | DBR_GR_DOUBLE | DBR_CTRL_DOUBLE | DBR_AL_DOUBLE};
};

struct metaENUM {
    DBRstatus
    DBRamsg
    DBRtime
    DBRutag
    DBRenumStrs

    // similar junk
    DBRunits
    DBRprecision
    DBRgrDouble
    DBRctrlDouble
    DBRalDouble

    enum {mask = DBR_STATUS | DBR_AMSG | DBR_TIME | DBR_UTAG | DBR_ENUM_STRS};
};

struct metaSTRING {
    DBRstatus
    DBRamsg
    DBRtime
    DBRutag

    // similar junk
    DBRenumStrs
    DBRunits
    DBRprecision
    DBRgrDouble
    DBRctrlDouble
    DBRalDouble

    enum {mask = DBR_STATUS | DBR_AMSG | DBR_TIME | DBR_UTAG};
};

void attachTime(pvTimeAlarm& pvm, const pvd::PVStructurePtr& pv)
{
#define FMAP(MNAME, PVT, FNAME, DBE) pvm.MNAME = pv->getSubFieldT<pvd::PVT>(FNAME); \
        pvm.mask ## DBE.set(pvm.MNAME->getFieldOffset())
    FMAP(status, PVInt, "alarm.status", ALARM);
    FMAP(severity, PVInt, "alarm.severity", ALARM);
    FMAP(message, PVString, "alarm.message", ALARM);
    FMAP(sec, PVLong, "timeStamp.secondsPastEpoch", ALWAYS);
    FMAP(nsec, PVInt, "timeStamp.nanoseconds", ALWAYS);
#ifdef HAVE_UTAG
    FMAP(userTag, PVInt, "timeStamp.userTag", ALWAYS);
#endif
#undef FMAP
}

static
pvd::shared_vector<const std::string> buildFormats()
{
    pvd::shared_vector<std::string> fmt;
    fmt.push_back("Default");
    fmt.push_back("String");
    fmt.push_back("Binary");
    fmt.push_back("Decimal");
    fmt.push_back("Hex");
    fmt.push_back("Exponential");
    fmt.push_back("Engineering");
    return pvd::freeze(fmt);
}

static const
pvd::shared_vector<const std::string> displayForms(buildFormats());

// lookup fields and populate pvCommon.  Non-existant fields will be NULL.
void attachMeta(pvCommon& pvm, const pvd::PVStructurePtr& pv)
{
    {
        pvd::PVStructurePtr fmt(pv->getSubField<pvd::PVStructure>("display.form"));
        if(fmt) {
            fmt->getSubFieldT<pvd::PVStringArray>("choices")->replace(displayForms);
        }
    }
    attachTime(pvm, pv);
#define FMAP(MNAME, PVT, FNAME, DBE) pvm.MNAME = pv->getSubField<pvd::PVT>(FNAME); \
        if(pvm.MNAME) pvm.mask ## DBE.set(pvm.MNAME->getFieldOffset())
    FMAP(displayHigh, PVDouble, "display.limitHigh", PROPERTY);
    FMAP(displayLow, PVDouble, "display.limitLow", PROPERTY);
    FMAP(controlHigh, PVDouble, "control.limitHigh", PROPERTY);
    FMAP(controlLow, PVDouble, "control.limitLow", PROPERTY);
    FMAP(egu, PVString, "display.units", PROPERTY);
    FMAP(desc, PVString, "display.description", PROPERTY);
    FMAP(prec, PVInt, "display.precision", PROPERTY);
    FMAP(fmt,  PVInt, "display.form.index", PROPERTY);
    FMAP(warnHigh, PVScalar, "valueAlarm.highWarningLimit", PROPERTY);
    FMAP(warnLow,  PVScalar, "valueAlarm.lowWarningLimit", PROPERTY);
    FMAP(alarmHigh, PVScalar, "valueAlarm.highAlarmLimit", PROPERTY);
    FMAP(alarmLow,  PVScalar, "valueAlarm.lowAlarmLimit", PROPERTY);
    FMAP(enumopts,  PVStringArray, "value.choices", PROPERTY);
#undef FMAP
}

template<typename PVX>
void attachAll(PVX& pvm, const pvd::PVStructurePtr& pv)
{
    pvm.value = pv->getSubField<typename PVX::pvd_type>("value.index");
    if(!pvm.value)
        pvm.value = pv->getSubFieldT<typename PVX::pvd_type>("value");
    const pvd::PVField *fld = pvm.value.get();
    pvm.maskVALUE.set(fld->getFieldOffset());
    for(;fld; fld = fld->getParent()) {
        // set field bit and all enclosing structure bits
        pvm.maskVALUEPut.set(fld->getFieldOffset());
    }
    pvm.maskVALUEPut.set(0);
    attachMeta(pvm, pv);
}

template<typename Meta>
void mapStatus(const Meta& meta, pvd::PVInt* status, pvd::PVString* message)
{
#ifdef HAVE_UTAG
    if(meta.amsg[0]!='\0') {
        message->put(meta.amsg);
    } else
#endif
    if(meta.status<ALARM_NSTATUS)
        message->put(epicsAlarmConditionStrings[meta.status]);
    else
        message->put("???");

    // Arbitrary mapping from DB status codes
    unsigned out;
    switch(meta.status) {
    case NO_ALARM:
        out = 0;
        break;
    case READ_ALARM:
    case WRITE_ALARM:
    case HIHI_ALARM:
    case HIGH_ALARM:
    case LOLO_ALARM:
    case LOW_ALARM:
    case STATE_ALARM:
    case COS_ALARM:
    case HW_LIMIT_ALARM:
        out = 1; // DEVICE
        break;
    case COMM_ALARM:
    case TIMEOUT_ALARM:
    case UDF_ALARM:
        out = 2; // DRIVER
        break;
    case CALC_ALARM:
    case SCAN_ALARM:
    case LINK_ALARM:
    case SOFT_ALARM:
    case BAD_SUB_ALARM:
        out = 3; // RECORD
        break;
    case DISABLE_ALARM:
    case SIMM_ALARM:
    case READ_ACCESS_ALARM:
    case WRITE_ACCESS_ALARM:
        out = 4; // DB
        break;
    default:
        out = 6; // UNDEFINED
    }

    status->put(out);
}


template<typename META>
void putMetaImpl(const pvTimeAlarm& pv, const META& meta)
{
    pvd::int32 nsec = meta.time.nsec;
    if(pv.nsecMask) {
        pv.userTag->put(nsec&pv.nsecMask);
        nsec &= ~pv.nsecMask;
#ifdef HAVE_UTAG
    } else {
        pv.userTag->put(meta.utag);
#endif
    }
    pv.nsec->put(nsec);    pv.sec->put(meta.time.secPastEpoch+POSIX_TIME_AT_EPICS_EPOCH);
}

void putTime(const pvTimeAlarm& pv, unsigned dbe, db_field_log *pfl)
{
    metaTIME meta;
    long options = (int)metaTIME::mask, nReq = 0;

    long status = dbChannelGet(pv.chan, dbChannelFinalFieldType(pv.chan), &meta, &options, &nReq, pfl);
    if(status)
        throw std::runtime_error("dbGet for meta fails");

    putMetaImpl(pv, meta);
    if(dbe&DBE_ALARM) {
        mapStatus(meta, pv.status.get(), pv.message.get());
        pv.severity->put(meta.severity);
    }
}

void putValue(dbChannel *chan, pvd::PVScalar* value, db_field_log *pfl)
{
    dbrbuf buf;
    long nReq = 1;

    long status = dbChannelGet(chan, dbChannelFinalFieldType(chan), &buf, NULL, &nReq, pfl);
    if(status)
        throw std::runtime_error("dbGet for meta fails");

    if(nReq==0) {
        // this was an actual max length 1 array, which has zero elements now.
        memset(&buf, 0, sizeof(buf));
    }

    switch(dbChannelFinalFieldType(chan)) {
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case DBR_##DBFTYPE: value->putFrom<PVATYPE>(buf.dbf_##DBFTYPE); break;
#define CASE_ENUM
#define CASE_SKIP_BOOL
#include "pv/typemap.h"
#undef CASE_ENUM
#undef CASE_SKIP_BOOL
#undef CASE
    case DBR_STRING:
        buf.dbf_STRING[sizeof(buf.dbf_STRING)-1] = '\0';
        value->putFrom<std::string>(buf.dbf_STRING);
        break;
    default:
        throw std::runtime_error("putValue unsupported DBR code");
    }
}

void getValue(dbChannel *chan, pvd::PVScalar* value)
{
    dbrbuf buf;

    switch(dbChannelFinalFieldType(chan)) {
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case DBR_##DBFTYPE: buf.dbf_##DBFTYPE = value->getAs<PVATYPE>(); break;
#define CASE_ENUM
#define CASE_SKIP_BOOL
#include "pv/typemap.h"
#undef CASE_ENUM
#undef CASE_SKIP_BOOL
#undef CASE
    case DBR_STRING:
    {
        std::string val(value->getAs<std::string>());
        strncpy(buf.dbf_STRING, val.c_str(), sizeof(buf.dbf_STRING));
        buf.dbf_STRING[sizeof(buf.dbf_STRING)-1] = '\0';
    }
        break;
    default:
        throw std::runtime_error("getValue unsupported DBR code");
    }

    long status = dbChannelPut(chan, dbChannelFinalFieldType(chan), &buf, 1);
    if(status)
        throw std::runtime_error("dbPut for meta fails");
}

void getValue(dbChannel *chan, pvd::PVScalarArray* value)
{
    short dbr = dbChannelFinalFieldType(chan);

    if(dbr!=DBR_STRING) {
        pvd::shared_vector<const void> buf;

        value->getAs(buf);
        long nReq = buf.size()/pvd::ScalarTypeFunc::elementSize(value->getScalarArray()->getElementType());

        long status = dbChannelPut(chan, dbr, buf.data(), nReq);
        if(status)
            throw std::runtime_error("dbChannelPut fails");

    } else {
        pvd::shared_vector<const std::string> buf;

        value->getAs(buf);

        std::vector<char> temp(buf.size()*MAX_STRING_SIZE);

        for(size_t i=0, N=buf.size(); i<N; i++)
        {
            strncpy(&temp[i*MAX_STRING_SIZE], buf[i].c_str(), MAX_STRING_SIZE-1);
            temp[i*MAX_STRING_SIZE + MAX_STRING_SIZE-1] = '\0';
        }

        long status = dbChannelPut(chan, dbr, &temp[0], buf.size());
        if(status)
            throw std::runtime_error("dbChannelPut fails");
    }
}

void putValue(dbChannel *chan, pvd::PVScalarArray* value, db_field_log *pfl)
{
    const short dbr = dbChannelFinalFieldType(chan);

    long nReq = dbChannelFinalElements(chan);
    const pvd::ScalarType etype = value->getScalarArray()->getElementType();

    if(dbr!=DBR_STRING) {

        pvd::shared_vector<void> buf(pvd::ScalarTypeFunc::allocArray(etype, nReq)); // TODO: pool?

        long status = dbChannelGet(chan, dbr, buf.data(), NULL, &nReq, pfl);
        if(status)
            throw std::runtime_error("dbChannelGet for value fails");

        buf.slice(0, nReq*pvd::ScalarTypeFunc::elementSize(etype));

        value->putFrom(pvd::freeze(buf));

    } else {
        std::vector<char> temp(nReq*MAX_STRING_SIZE);

        long status = dbChannelGet(chan, dbr, &temp[0], NULL, &nReq, pfl);
        if(status)
            throw std::runtime_error("dbChannelGet for value fails");

        pvd::shared_vector<std::string> buf(nReq);
        for(long i=0; i<nReq; i++) {
            temp[i*MAX_STRING_SIZE + MAX_STRING_SIZE-1] = '\0';
            buf[i] = std::string(&temp[i*MAX_STRING_SIZE]);
        }

        value->putFrom(pvd::freeze(buf));
    }
}
template<typename META>
void putMeta(const pvCommon& pv, unsigned dbe, db_field_log *pfl)
{
    META meta;
    long options = (int)META::mask, nReq = 0;
    dbCommon *prec = dbChannelRecord(pv.chan);

    long status = dbChannelGet(pv.chan, dbChannelFinalFieldType(pv.chan), &meta, &options, &nReq, pfl);
    if(status)
        throw std::runtime_error("dbGet for meta fails");

    putMetaImpl(pv, meta);
#define FMAP(MNAME, FNAME) pv.MNAME->put(meta.FNAME)
    if(dbe&DBE_ALARM) {
        mapStatus(meta, pv.status.get(), pv.message.get());
        FMAP(severity, severity);
    }
    if(dbe&DBE_PROPERTY) {
#undef FMAP
        if(pv.desc) pv.desc->put(prec->desc);
#define FMAP(MASK, MNAME, FNAME) if(META::mask&(MASK) && pv.MNAME) pv.MNAME->put(meta.FNAME)
        FMAP(DBR_GR_DOUBLE, displayHigh, upper_disp_limit);
        FMAP(DBR_GR_DOUBLE, displayLow, lower_disp_limit);
        FMAP(DBR_CTRL_DOUBLE, controlHigh, upper_ctrl_limit);
        FMAP(DBR_CTRL_DOUBLE, controlLow, lower_ctrl_limit);
        FMAP(DBR_GR_DOUBLE, egu, units);
#undef FMAP
        if(META::mask&DBR_PRECISION && pv.prec) {
            pv.prec->put(pvd::int32(meta.precision.dp));
        }
#define FMAP(MASK, MNAME, FNAME) if(META::mask&(MASK) && pv.MNAME) pv.MNAME->putFrom(meta.FNAME)
        // not handling precision until I get a better idea of what 'format' is supposed to be...
        //FMAP(prec,  PVScalar, "display.form", PROPERTY);
        FMAP(DBR_AL_DOUBLE, warnHigh, upper_warning_limit);
        FMAP(DBR_AL_DOUBLE, warnLow,  lower_warning_limit);
        FMAP(DBR_AL_DOUBLE, alarmHigh, upper_alarm_limit);
        FMAP(DBR_AL_DOUBLE, alarmLow,  lower_alarm_limit);
#undef FMAP
        if(pv.enumopts) {
            pvd::shared_vector<std::string> strs(meta.no_str);
            for(size_t i=0; i<strs.size(); i++)
            {
                meta.strs[i][sizeof(meta.strs[i])-1] = '\0';
                strs[i] = meta.strs[i];
            }
            pv.enumopts->replace(pvd::freeze(strs));
        }
    }
}

template<typename PVC, typename META>
void putAll(const PVC &pv, unsigned dbe, db_field_log *pfl)
{
    if(dbe&(DBE_VALUE|DBE_ARCHIVE)) {
        putValue(pv.chan, pv.value.get(), pfl);
    }
    if(!(dbe&DBE_PROPERTY)) {
        putTime(pv, dbe, pfl);
    } else {
        putMeta<META>(pv, dbe, pfl);
    }
}

void findNSMask(pvTimeAlarm& pvmeta, pdbRecordIterator& info, const epics::pvData::PVStructurePtr& pvalue)
{
    const char *UT = info.info("Q:time:tag");
    if(UT && strncmp(UT, "nsec:lsb:", 9)==0) {
        try{
            pvmeta.nsecMask = epics::pvData::castUnsafe<pvd::uint32>(std::string(&UT[9]));
        }catch(std::exception& e){
            pvmeta.nsecMask = 0;
            std::cerr<<info.name()<<" : Q:time:tag nsec:lsb: requires a number not '"<<UT[9]<<"'\n";
        }
    }
    if(pvmeta.nsecMask>0 && pvmeta.nsecMask<=32) {
        pvmeta.userTag = pvalue->getSubField<pvd::PVInt>("timeStamp.userTag");
        if(!pvmeta.userTag) {
            pvmeta.nsecMask = 0; // struct doesn't have userTag
        } else {
            pvd::uint64 mask = (1<<pvmeta.nsecMask)-1;
            pvmeta.nsecMask = mask;
            pvmeta.maskALWAYS.set(pvmeta.userTag->getFieldOffset());
        }
    } else
        pvmeta.nsecMask = 0;
}

void findFormat(pvTimeAlarm& pvmeta, pdbRecordIterator& info, const epics::pvData::PVStructurePtr& pvalue)
{
    const char *FMT = info.info("Q:form");
    if(FMT) {
        pvd::PVScalarPtr fmt(pvalue->getSubField<pvd::PVScalar>("display.form.index"));
        if(fmt) {
            bool found = false;
            for(size_t i=0; !found && i<displayForms.size(); i++) {
                if((found=(displayForms[i]==FMT)))
                    fmt->putFrom<pvd::uint32>(i);
            }
            if(!found) {
                try {
                    fmt->putFrom(std::string(FMT)); // attempt to parse as number
                }catch(std::exception& e){
                    errlogPrintf("%s: info(Q:form, \"%s\") is not known format: %s\n", info.name(), FMT, e.what());
                }
            }
        }
    }
}

pvd::Status checkDISP(dbChannel *chan)
{
    dbCommon *prec = dbChannelRecord(chan);
    pvd::Status ret;
    if(prec->disp && dbChannelField(chan)!=&prec->disp)
        ret = pvd::Status::error("Put Disabled");
    return ret;
}

template<typename PVX, typename META>
struct PVIFScalarNumeric : public PVIF
{
    PVX pvmeta;
    const epics::pvData::PVStructurePtr pvalue;

    PVIFScalarNumeric(dbChannel *ch, const epics::pvData::PVFieldPtr& p, pvd::PVField *enclosing)
        :PVIF(ch)
        ,pvalue(std::tr1::dynamic_pointer_cast<pvd::PVStructure>(p))
    {
        if(!pvalue)
            throw std::runtime_error("Must attach to structure");

        pvmeta.chan = ch;
        attachAll<PVX>(pvmeta, pvalue);
        if(enclosing) {
            size_t bit = enclosing->getFieldOffset();
            // we are inside a structure array or similar with only one bit for all ours fields
            pvmeta.maskALWAYS.clear();
            pvmeta.maskALWAYS.set(bit);
            pvmeta.maskVALUE.clear();
            pvmeta.maskVALUE.set(bit);
            pvmeta.maskALARM.clear();
            pvmeta.maskALARM.set(bit);
            pvmeta.maskPROPERTY.clear();
            pvmeta.maskPROPERTY.set(bit);
            pvmeta.maskVALUEPut.clear();
            pvmeta.maskVALUEPut.set(0);
            pvmeta.maskVALUEPut.set(bit);
        }
        pdbRecordIterator info(chan);
        findNSMask(pvmeta, info, pvalue);
        findFormat(pvmeta, info, pvalue);
    }
    virtual ~PVIFScalarNumeric() {}

    virtual void put(epics::pvData::BitSet& mask, unsigned dbe, db_field_log *pfl) OVERRIDE FINAL
    {
        try{
            putAll<PVX, META>(pvmeta, dbe, pfl);
            mask |= pvmeta.maskALWAYS;
            if(dbe&(DBE_VALUE|DBE_ARCHIVE))
                mask |= pvmeta.maskVALUE;
            if(dbe&DBE_ALARM)
                mask |= pvmeta.maskALARM;
            if(dbe&DBE_PROPERTY)
                mask |= pvmeta.maskPROPERTY;
        }catch(...){
            pvmeta.severity->put(3);
            mask |= pvmeta.maskALARM;
            throw;
        }
    }

    virtual pvd::Status get(const epics::pvData::BitSet& mask, proc_t proc, bool permit) OVERRIDE FINAL
    {
        pvd::Status ret = checkDISP(chan);
        if(!ret)
            return ret;

        bool newval = mask.logical_and(pvmeta.maskVALUEPut);
        if(newval) {
            if(permit)
                getValue(pvmeta.chan, pvmeta.value.get());
            else
                ret = pvd::Status::error("Put not permitted");
        }
        if(newval || proc==PVIF::ProcForce) {
            if(permit)
                ret = PVIF::get(mask, proc);
            else
                ret = pvd::Status::error("Process not permitted");
        }
        return ret;
    }

    virtual unsigned dbe(const epics::pvData::BitSet& mask) OVERRIDE FINAL
    {
        unsigned ret = 0;
        if(mask.logical_and(pvmeta.maskVALUE))
            ret |= DBE_VALUE;
        if(mask.logical_and(pvmeta.maskALARM))
            ret |= DBE_ALARM;
        if(mask.logical_and(pvmeta.maskPROPERTY))
            ret |= DBE_PROPERTY;
        return ret;
    }
};

} // namespace

static
pvd::ScalarType DBR2PVD(short dbr)
{
    switch(dbr) {
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case DBR_##DBFTYPE: return pvd::pv##PVACODE;
#define CASE_ENUM
#define CASE_SKIP_BOOL
#include "pv/typemap.h"
#undef CASE_ENUM
#undef CASE_SKIP_BOOL
#undef CASE
    case DBF_STRING: return pvd::pvString;
    }
    throw std::invalid_argument("Unsupported DBR code");
}

short PVD2DBR(pvd::ScalarType pvt)
{
    switch(pvt) {
#define CASE(BASETYPE, PVATYPE, DBFTYPE, PVACODE) case pvd::pv##PVACODE: return DBR_##DBFTYPE;
#ifndef USE_INT64
#  define CASE_SQUEEZE_INT64
#endif
#include "pv/typemap.h"
#ifndef USE_INT64
#  undef CASE_SQUEEZE_INT64
#endif
#undef CASE
    case pvd::pvString: return DBF_STRING;
    }
    return -1;
}

static
pvd::StructureConstPtr buildTimeStamp()
{
    return pvd::FieldBuilder::begin()
                    ->add("secondsPastEpoch", pvd::pvLong)
                    ->add("nanoseconds", pvd::pvInt)
                    ->add("userTag", pvd::pvInt)
                  ->createStructure();
}

epics::pvData::FieldConstPtr
ScalarBuilder::dtype()
{
    if(!channel)
        throw std::runtime_error("+type:\"scalar\" requires +channel:");

    short dbr = dbChannelFinalFieldType(channel);
    const long maxelem = dbChannelFinalElements(channel);
    const pvd::ScalarType pvt = DBR2PVD(dbr);

    if(INVALID_DB_REQ(dbr))
        throw std::invalid_argument("DBF code out of range");

    if(maxelem!=1 && dbr==DBR_ENUM)
        dbr = DBF_SHORT;

    pvd::FieldBuilderPtr builder(pvd::getFieldCreate()->createFieldBuilder());
    pvd::StandardFieldPtr standard(pvd::getStandardField());

    if(dbr==DBR_ENUM)
        builder = builder->setId("epics:nt/NTEnum:1.0")
                         ->addNestedStructure("value")
                            ->setId("enum_t")
                            ->add("index", pvd::pvInt)
                            ->addArray("choices", pvd::pvString)
                         ->endNested();
    else if(maxelem==1)
        builder = builder->setId("epics:nt/NTScalar:1.0")
                         ->add("value", pvt);
    else
        builder = builder->setId("epics:nt/NTScalarArray:1.0")
                         ->addArray("value", pvt);

    builder = builder->add("alarm", standard->alarm())
                     ->add("timeStamp", buildTimeStamp());

    if(dbr!=DBR_ENUM) {
        builder = builder->addNestedStructure("display")
                            ->add("limitLow", pvd::pvDouble)
                            ->add("limitHigh", pvd::pvDouble)
                            ->add("description", pvd::pvString)
                            ->add("units", pvd::pvString)
                            ->add("precision", pvd::pvInt)
                            ->addNestedStructure("form")
                                ->setId("enum_t")
                                ->add("index", pvd::pvInt)
                                ->addArray("choices", pvd::pvString)
                            ->endNested()
                         ->endNested()
                         ->add("control", standard->control());

        if(dbr!=DBR_STRING)
            builder = builder->add("valueAlarm", standard->doubleAlarm());
    }

    return builder->createStructure();
}

PVIF*
ScalarBuilder::attach(const epics::pvData::PVStructurePtr& root, const FieldName& fldname)
{
    if(!channel)
        throw std::runtime_error("+type:\"scalar\" requires +channel:");
    pvd::PVField *enclosing = 0;
    pvd::PVFieldPtr fld(fldname.lookup(root, &enclosing));

    const short dbr = dbChannelFinalFieldType(channel);
    const long maxelem = dbChannelFinalElements(channel);

    if(maxelem==1) {
        switch(dbr) {
        case DBR_CHAR:
        case DBR_UCHAR:
        case DBR_SHORT:
        case DBR_USHORT:
        case DBR_LONG:
        case DBR_ULONG:
#ifdef USE_INT64
        case DBR_INT64:
        case DBR_UINT64:
#endif
            return new PVIFScalarNumeric<pvScalar, metaDOUBLE>(channel, fld, enclosing);
        case DBR_FLOAT:
        case DBR_DOUBLE:
            return new PVIFScalarNumeric<pvScalar, metaDOUBLE>(channel, fld, enclosing);
        case DBR_ENUM:
            return new PVIFScalarNumeric<pvScalar, metaENUM>(channel, fld, enclosing);
        case DBR_STRING:
            return new PVIFScalarNumeric<pvScalar, metaSTRING>(channel, fld, enclosing);
        }
    } else {
        switch(dbr) {
        case DBR_CHAR:
        case DBR_UCHAR:
        case DBR_SHORT:
        case DBR_ENUM:
        case DBR_USHORT:
        case DBR_LONG:
        case DBR_ULONG:
        case DBR_STRING:
        case DBR_FLOAT:
#ifdef USE_INT64
        case DBR_INT64:
        case DBR_UINT64:
#endif
        case DBR_DOUBLE:
            return new PVIFScalarNumeric<pvArray, metaDOUBLE>(channel, fld, enclosing);
        }
    }

    throw std::invalid_argument("Channel has invalid/unsupported DBR type");
}

namespace {
template<class PVD>
struct PVIFPlain : public PVIF
{
    const typename PVD::shared_pointer field;
    size_t fieldOffset;
    dbChannel * const channel;

    PVIFPlain(dbChannel *channel, const epics::pvData::PVFieldPtr& fld, epics::pvData::PVField* enclosing=0)
        :PVIF(channel)
        ,field(std::tr1::static_pointer_cast<PVD>(fld))
        ,channel(channel)
    {
        if(!field)
            throw std::logic_error("PVIFPlain attached type mis-match");
        if(enclosing)
            fieldOffset = enclosing->getFieldOffset();
        else
            fieldOffset = field->getFieldOffset();
    }

    virtual ~PVIFPlain() {}

    virtual void put(epics::pvData::BitSet& mask, unsigned dbe, db_field_log *pfl) OVERRIDE FINAL
    {
        if(dbe&DBE_VALUE) {
            putValue(channel, field.get(), pfl);
            mask.set(fieldOffset);
        }
    }

    virtual pvd::Status get(const epics::pvData::BitSet& mask, proc_t proc, bool permit) OVERRIDE FINAL
    {
        pvd::Status ret = checkDISP(chan);
        if(!ret)
            return ret;

        bool newval = mask.get(fieldOffset);
        if(newval) {
            if(permit)
                getValue(channel, field.get());
            else
                ret = pvd::Status::error("Put not permitted");
        }
        if(newval || proc==PVIF::ProcForce) {
            if(permit)
                ret = PVIF::get(mask, proc);
            else
                ret = pvd::Status::error("Process not permitted");
        }
        return ret;
    }

    virtual unsigned dbe(const epics::pvData::BitSet& mask) OVERRIDE FINAL
    {
        // TODO: figure out how to handle various intermidiate compressed
        //       bitSet and enclosing.
        //       Until then check only also for wildcard bit (0).
        if(mask.get(fieldOffset) || mask.get(0))
            return DBE_VALUE;
        return 0;
    }
};

struct PlainBuilder : public PVIFBuilder
{
    explicit PlainBuilder(dbChannel* chan) :PVIFBuilder(chan) {}
    virtual ~PlainBuilder() {}

    // fetch the structure description
    virtual epics::pvData::FieldConstPtr dtype() OVERRIDE FINAL {
        if(!channel)
            throw std::runtime_error("+type:\"plain\" requires +channel:");

        const short dbr = dbChannelFinalFieldType(channel);
        const long maxelem = dbChannelFinalElements(channel);
        const pvd::ScalarType pvt = DBR2PVD(dbr);

        if(INVALID_DB_REQ(dbr))
            throw std::invalid_argument("DBF code out of range");

        if(maxelem==1)
            return pvd::getFieldCreate()->createScalar(pvt);
        else
            return pvd::getFieldCreate()->createScalarArray(pvt);
    }

    // Attach to a structure instance.
    // must be of the type returned by dtype().
    // need not be the root structure
    virtual PVIF* attach(const epics::pvData::PVStructurePtr& root,
                         const FieldName& fldname) OVERRIDE FINAL
    {
        if(!channel)
            throw std::runtime_error("+type:\"plain\" requires +channel:");
        const long maxelem = dbChannelFinalElements(channel);

        pvd::PVField *enclosing = 0;
        pvd::PVFieldPtr fld(fldname.lookup(root, &enclosing));

        if(maxelem==1)
            return new PVIFPlain<pvd::PVScalar>(channel, fld, enclosing);
        else
            return new PVIFPlain<pvd::PVScalarArray>(channel, fld, enclosing);
    }
};

struct AnyScalarBuilder : public PVIFBuilder
{
    explicit AnyScalarBuilder(dbChannel* chan) :PVIFBuilder(chan) {}
    virtual ~AnyScalarBuilder() {}

    // fetch the structure description
    virtual epics::pvData::FieldConstPtr dtype() OVERRIDE FINAL {
        (void)channel; //ignored
        return pvd::getFieldCreate()->createVariantUnion();
    }

    // Attach to a structure instance.
    // must be of the type returned by dtype().
    // need not be the root structure
    virtual PVIF* attach(const epics::pvData::PVStructurePtr& root,
                         const FieldName& fldname) OVERRIDE FINAL
    {
        if(!channel)
            throw std::runtime_error("+type:\"any\" requires +channel:");
        pvd::PVDataCreatePtr create(pvd::getPVDataCreate());
        const short dbr = dbChannelFinalFieldType(channel);
        const long maxelem = dbChannelFinalElements(channel);
        const pvd::ScalarType pvt = DBR2PVD(dbr);

        pvd::PVField *enclosing = 0;
        pvd::PVFieldPtr fld(fldname.lookup(root, &enclosing));

        pvd::PVUnion *value = dynamic_cast<pvd::PVUnion*>(fld.get());
        if(!value)
            throw std::logic_error("Mis-matched attachment point");

        pvd::PVFieldPtr arr(value->get());
        if(!arr) {
            if(maxelem==1)
                arr = create->createPVScalar(pvt);
            else
                arr = create->createPVScalarArray(pvt);
            value->set(arr);
        }

        if(maxelem==1)
            return new PVIFPlain<pvd::PVScalar>(channel, arr, enclosing ? enclosing : arr.get());
        else
            return new PVIFPlain<pvd::PVScalarArray>(channel, arr, enclosing ? enclosing : arr.get());
    }

};

struct PVIFMeta : public PVIF
{
    pvTimeAlarm meta;

    PVIFMeta(dbChannel *channel, const epics::pvData::PVFieldPtr& fld, epics::pvData::PVField* enclosing=0)
        :PVIF(channel)
    {
        pvd::PVStructurePtr field(std::tr1::dynamic_pointer_cast<pvd::PVStructure>(fld));
        if(!field)
            throw std::logic_error("PVIFMeta attached type mis-match");
        meta.chan = channel;
        pdbRecordIterator info(chan);
        attachTime(meta, field);
        findNSMask(meta, info, field);
        findFormat(meta, info, field);
        if(enclosing) {
            meta.maskALWAYS.clear();
            meta.maskALWAYS.set(enclosing->getFieldOffset());
            meta.maskALARM.clear();
            meta.maskALARM.set(enclosing->getFieldOffset());
        }
    }

    virtual ~PVIFMeta() {}

    virtual void put(epics::pvData::BitSet& mask, unsigned dbe, db_field_log *pfl) OVERRIDE FINAL
    {
        mask |= meta.maskALWAYS;
        if(dbe&DBE_ALARM)
            mask |= meta.maskALARM;

        putTime(meta, dbe, pfl);
    }

    virtual pvd::Status get(const epics::pvData::BitSet& mask, proc_t proc, bool permit) OVERRIDE FINAL
    {
        // can't put time/alarm
        if(mask.logical_and(meta.maskALARM))
            return pvd::Status::warn("Put to meta field ignored");
        return pvd::Status::Ok;
    }

    virtual unsigned dbe(const epics::pvData::BitSet& mask) OVERRIDE FINAL
    {
        if(mask.logical_and(meta.maskALARM))
            return DBE_ALARM;
        return 0;
    }
};

struct MetaBuilder : public PVIFBuilder
{
    explicit MetaBuilder(dbChannel* chan) :PVIFBuilder(chan) {}
    virtual ~MetaBuilder() {}

    // fetch the structure description
    virtual epics::pvData::FieldConstPtr dtype() OVERRIDE FINAL {
        throw std::logic_error("Don't call me");
    }

    virtual epics::pvData::FieldBuilderPtr dtype(epics::pvData::FieldBuilderPtr& builder,
                                                 const std::string& fld) OVERRIDE FINAL
    {
        pvd::StandardFieldPtr std(pvd::getStandardField());
        if(fld.empty()) {
            return builder->add("alarm", std->alarm())
                          ->add("timeStamp", buildTimeStamp());
        } else {
            return builder->addNestedStructure(fld)
                                ->add("alarm", std->alarm())
                                ->add("timeStamp", buildTimeStamp())
                           ->endNested();
        }
    }

    // Attach to a structure instance.
    // must be of the type returned by dtype().
    // need not be the root structure
    virtual PVIF* attach(const epics::pvData::PVStructurePtr& root,
                         const FieldName& fldname) OVERRIDE FINAL
    {
        if(!channel)
            throw std::runtime_error("+type:\"meta\" requires +channel:");

        pvd::PVField *enclosing = 0;
        pvd::PVFieldPtr fld(fldname.lookup(root, &enclosing));

        return new PVIFMeta(channel, fld, enclosing);
    }

};

struct PVIFProc : public PVIF
{
    PVIFProc(dbChannel *channel) :PVIF(channel) {}

    virtual void put(epics::pvData::BitSet& mask, unsigned dbe, db_field_log *pfl) OVERRIDE FINAL
    {
        // nothing to get
    }

    virtual pvd::Status get(const epics::pvData::BitSet& mask, proc_t proc, bool permit) OVERRIDE FINAL
    {
        // always process (if permitted)
        return PVIF::get(mask, PVIF::ProcForce, permit);
    }

    virtual unsigned dbe(const epics::pvData::BitSet& mask) OVERRIDE FINAL
    {
        return 0;
    }
};

struct ProcBuilder : public PVIFBuilder
{
    explicit ProcBuilder(dbChannel* chan) :PVIFBuilder(chan) {}
    virtual ~ProcBuilder() {}

    // fetch the structure description
    virtual epics::pvData::FieldConstPtr dtype() OVERRIDE FINAL {
        throw std::logic_error("Don't call me");
    }

    virtual epics::pvData::FieldBuilderPtr dtype(epics::pvData::FieldBuilderPtr& builder,
                                                 const std::string& fld) OVERRIDE FINAL
    {
        // invisible
        return builder;
    }
    virtual PVIF* attach(const epics::pvData::PVStructurePtr& root,
                         const FieldName& fldname) OVERRIDE FINAL
    {
        if(!channel)
            throw std::runtime_error("+type:\"proc\" requires +channel:");

        return new PVIFProc(channel);
    }
};

struct PVIFNoOp : public PVIF
{
    PVIFNoOp(dbChannel *channel) :PVIF(channel) {}

    virtual void put(epics::pvData::BitSet& mask, unsigned dbe, db_field_log *pfl) OVERRIDE FINAL
    {}

    virtual pvd::Status get(const epics::pvData::BitSet& mask, proc_t proc, bool permit) OVERRIDE FINAL
    {
        return pvd::Status();
    }

    virtual unsigned dbe(const epics::pvData::BitSet& mask) OVERRIDE FINAL
    {
        return 0;
    }
};

struct IDBuilder : public PVIFBuilder
{
    explicit IDBuilder(dbChannel* chan) :PVIFBuilder(chan) {}
    virtual ~IDBuilder() {}

    // fetch the structure description
    virtual epics::pvData::FieldConstPtr dtype() OVERRIDE FINAL {
        throw std::logic_error("Don't call me");
    }

    virtual epics::pvData::FieldBuilderPtr dtype(epics::pvData::FieldBuilderPtr& builder,
                                                 const std::string& fld) OVERRIDE FINAL
    {
        // caller has already done builder->setId(...)
        return builder;
    }
    virtual PVIF* attach(const epics::pvData::PVStructurePtr& root,
                         const FieldName& fldname) OVERRIDE FINAL
    {
        return new PVIFNoOp(channel);
    }
};

}//namespace

pvd::Status PVIF::get(const epics::pvData::BitSet& mask, proc_t proc, bool permit)
{
    dbCommon *precord = dbChannelRecord(chan);

    bool tryproc = proc!=ProcPassive ? proc==ProcForce :
                                       dbChannelField(chan) == &precord->proc ||
                                         (dbChannelFldDes(chan)->process_passive &&
                                          precord->scan == 0);

    pvd::Status ret;

    if (tryproc) {
        if (!permit) {
            return pvd::Status::error("Process not permitted");

        } else if (precord->pact) {
            if (precord->tpro)
                printf("%s: Active %s\n",
                       epicsThreadGetNameSelf(), precord->name);
            precord->rpro = TRUE;
        } else {
            /* indicate that dbPutField called dbProcess */
            precord->putf = TRUE;
            long err = dbProcess(precord);
            if(err) {
                char buf[32];
                errSymLookup(err, buf, sizeof(buf));
                std::ostringstream msg;
                msg<<"process error : "<<buf;
                ret = pvd::Status::error(msg.str());
            }
        }
    }

    return ret;
}

epics::pvData::FieldBuilderPtr
PVIFBuilder::dtype(epics::pvData::FieldBuilderPtr& builder,
                   const std::string &fld)
{
    if(fld.empty())
        throw std::runtime_error(SB()<<"Can't attach +type "<<typeid(*this).name()<<" to root");

    epics::pvData::FieldConstPtr ftype(this->dtype());
    if(ftype)
        builder = builder->add(fld, ftype);

    return builder;
}

PVIFBuilder* PVIFBuilder::create(const std::string& type, dbChannel* chan)
{
    if(type.empty() || type=="scalar")
        return new ScalarBuilder(chan);
    else if(type=="plain")
        return new PlainBuilder(chan);
    else if(type=="any")
        return new AnyScalarBuilder(chan);
    else if(type=="meta")
        return new MetaBuilder(chan);
    else if(type=="proc")
        return new ProcBuilder(chan);
    else if(type=="structure")
        return new IDBuilder(chan);
    else
        throw std::runtime_error(std::string("Unknown +type=")+type);
}
