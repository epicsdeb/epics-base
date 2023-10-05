/* printer.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <stdio.h>
#include <ctype.h>
#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#define HAVE_ISATTY
#endif

#include <deque>

#include <epicsTime.h>
#include <epicsString.h>

#define epicsExportSharedSymbols
#include <pv/bitSet.h>
#include <pv/pvData.h>
#include <pv/json.h>

namespace epics { namespace pvData {

namespace format
{
static int indent_index = std::ios_base::xalloc();

long& indent_value(std::ios_base& ios)
{
    return ios.iword(indent_index);
}

std::ostream& operator<<(std::ostream& os, indent_level const& indent)
{
    indent_value(os) = indent.level;
    return os;
}

std::ostream& operator<<(std::ostream& os, indent const&)
{
    long il = indent_value(os);
    for(long i=0, spaces = il * 4; i<spaces; i++)
        os.put(' ');
    return os;
}

array_at_internal operator<<(std::ostream& str, array_at const& manip)
{
    return array_at_internal(manip.index, str);
}
}

}} //epics::pvData
namespace {
using namespace epics::pvData;

bool useEscapes(std::ostream& strm) {
    FILE *fp = 0;
    // TODO: a better way to do this...
    if(&std::cout==&strm)
        fp = stdout;
    if(&std::cerr==&strm)
        fp = stderr;
    if(!fp) return false;
#ifdef HAVE_ISATTY
    // check $TERM as well?
    return isatty(fileno(fp))==1;
#else
    // TODO: in theory windows 10 can be made to understand escapes as well
    return false;
#endif
}


void printAlarmTx(std::ostream& strm, const PVStructure& sub)
{
    PVScalar::const_shared_pointer pvSeverity(sub.getSubField<PVInt>("severity"));
    PVScalar::const_shared_pointer pvStatus(sub.getSubField<PVInt>("status"));
    PVString::const_shared_pointer pvMessage(sub.getSubField<PVString>("message"));

    switch(pvSeverity ? pvSeverity->getAs<int32>() : 0) {
    case 0: return; // nothing more to do here...
    case 1: strm<<"MINOR "; break;
    case 2: strm<<"MAJOR "; break;
    case 3: strm<<"INVALID "; break;
    case 4: strm<<"UNDEFINED "; break;
    default: strm<<pvSeverity->getAs<int32>()<<' ';
    }

    switch(pvStatus ? pvStatus->getAs<int32>() : 0) {
    case 0: break;
    case 1: strm<<"DEVICE "; break;
    case 2: strm<<"DRIVER "; break;
    case 3: strm<<"RECORD "; break;
    case 4: strm<<"DB "; break;
    case 5: strm<<"CONF "; break;
    case 6: strm<<"UNDEFINED "; break;
    case 7: strm<<"CLIENT "; break;
    default: strm<<pvStatus->getAs<int32>()<<' ';
    }

    if(pvMessage && !pvMessage->get().empty())
        strm<<pvMessage->get()<<' ';
}


void printAlarmT(std::ostream& strm, const PVStructure& top)
{
    PVStructure::const_shared_pointer sub(top.getSubField<PVStructure>("alarm"));
    if(sub)
        printAlarmTx(strm, *sub);
}

void printTimeTx(std::ostream& strm, const PVStructure& tsubop)
{
    char timeText[32];
    epicsTimeStamp epicsTS;

    PVScalar::const_shared_pointer secf(tsubop.getSubField<PVScalar>("secondsPastEpoch")),
                                   nsecf(tsubop.getSubField<PVScalar>("nanoseconds")),
                                   tagf(tsubop.getSubField<PVScalar>("userTag"));

    epicsTS.secPastEpoch = secf ? secf->getAs<int64>() : 0;
    epicsTS.nsec = nsecf ? nsecf->getAs<int32>() : 0;

    if(epicsTS.secPastEpoch > POSIX_TIME_AT_EPICS_EPOCH)
        epicsTS.secPastEpoch -= POSIX_TIME_AT_EPICS_EPOCH;
    else
        epicsTS.secPastEpoch = 0;

    epicsTimeToStrftime(timeText, sizeof(timeText), "%Y-%m-%d %H:%M:%S.%03f", &epicsTS);
    strm <<std::setw(24) <<std::left <<timeText <<' ';
    if (tagf) {
        int64 tagv = tagf->getAs<int64>();
        if(tagv)
            strm << tagv << ' ';
    }
}


void printTimeT(std::ostream& strm, const PVStructure& top)
{
    PVStructure::const_shared_pointer sub(top.getSubField<PVStructure>("timeStamp"));
    if(sub)
        printTimeTx(strm, *sub);
}

bool printEnumT(std::ostream& strm, const PVStructure& top, bool fromtop)
{
    PVStructure::const_shared_pointer value;
    if(fromtop) {
        value = top.getSubField<PVStructure>("value");
    } else {
        value = std::tr1::static_pointer_cast<const PVStructure>(top.shared_from_this());
    }
    PVScalar::const_shared_pointer idx(value->getSubField<PVScalar>("index"));
    PVStringArray::const_shared_pointer choices(value->getSubField<PVStringArray>("choices"));
    if(!idx || !choices) return false;

    if(fromtop) {
        strm<<format::indent();
        printTimeT(strm, top);
        printAlarmT(strm, top);
    }

    PVStringArray::const_svector ch(choices->view());
    uint32 I = idx->getAs<uint32>();
    strm<<'('<<I<<')';
    if(I>=ch.size()) {
        strm<<" <undefined>";
    } else {
        strm<<' '<<maybeQuote(ch[I]);
    }
    return true;
}

void csvEscape(std::string& S)
{
    // concise, not particularly efficient...
    std::string temp(escape(S).style(escape::CSV).str());
    if(S.find_first_of("\" ,\\")!=S.npos) {// only quote if necessary (stupid Excel)
        std::string temp2;
        temp2.reserve(temp.size()+2);
        temp2.push_back('\"');
        temp2 += temp;
        temp2.push_back('\"');
        temp2.swap(temp);
    }
    S.swap(temp);
}

bool printTable(std::ostream& strm, const PVStructure& top)
{
    PVStructure::const_shared_pointer cf(top.getSubField<PVStructure>("value"));
    if(!cf) return false;

    {
        const FieldConstPtrArray& fields = cf->getStructure()->getFields();
        if(fields.empty()) return false;
        for(size_t i=0, N=fields.size(); i<N; i++) {
            if(fields[i]->getType()!=scalarArray)
                return false; // cowardly refusing to handle anything else...
        }
    }

    // maybe output a line with meta-data
    {
        bool havets = !!top.getSubField("timeStamp");
        bool haveal = !!top.getSubField("alarm");
        if(havets || haveal)
            strm<<format::indent();
        if(havets) {
            printTimeT(strm, top);
            strm<<' ';
        }
        if(haveal) {
            printAlarmT(strm, top);
            strm<<' ';
        }
        strm<<'\n';
    }

    PVStringArray::svector labels;
    {
        PVStringArray::const_shared_pointer lf(top.getSubField<PVStringArray>("labels"));
        if(lf) {
            PVStringArray::const_svector L(lf->view());
            labels = thaw(L);
        }
    }

    const PVFieldPtrArray& columns = cf->getPVFields();
    std::vector<shared_vector<std::string> > coldat(columns.size());

    std::vector<size_t> widths(columns.size());
    labels.reserve(columns.size());

    size_t nrows = size_t(-1);

    for(size_t i=0, N=columns.size(); i<N; i++) {
        if(i>=labels.size()) {
            labels.push_back(cf->getStructure()->getFieldName(i));
        } else {
            csvEscape(labels[i]);
        }
        widths[i] = labels[i].size();

        {
            shared_vector<const std::string> ro;
            static_cast<const PVScalarArray*>(columns[i].get())->getAs(ro);
            coldat[i] = thaw(ro);
        }

        // truncate if some columns are longer than others
        nrows = std::min(nrows, coldat[i].size());

        for(size_t j=0, M=coldat[i].size(); j<M; j++) {
            csvEscape(coldat[i][j]);
            widths[i] = std::max(widths[i], coldat[i][j].size());
        }
    }

    // output header line
    strm<<format::indent();
    for(size_t c=0, N=coldat.size(); c<N; c++) {
        strm<<std::setw(widths[c])<<std::right<<labels[c];
        if(c+1!=N) {
            strm<<' ';
        }
    }
    strm<<'\n';

    for(size_t r=0; r<nrows; r++) {
        strm<<format::indent();
        for(size_t c=0, N=coldat.size(); c<N; c++) {
            strm<<std::setw(widths[c])<<std::right<<coldat[c][r];
            if(c+1!=N)
                strm<<' ';
        }
        strm<<'\n';
    }

    return true;
}

void expandBS(const PVStructure& top, BitSet& mask, bool parents) {
    if(mask.get(0)) { // special handling because getSubField(0) not allowed
        // wildcard
        for(size_t idx=1, N=top.getNumberFields(); idx<N; idx++) {
            mask.set(idx);
        }

    } else {
        for(int32 idx = mask.nextSetBit(0), N=top.getNumberFields(); idx>=0 && idx<N; idx=mask.nextSetBit(idx+1)) {
            PVField::const_shared_pointer fld = top.getSubFieldT(idx);

            // look forward and mark all children
            for(size_t i=idx+1, N=fld->getNextFieldOffset(); i<N; i++)
                mask.set(i);

            if(parents) {
                // look back and mark all parents
                // we've already stepped past all parents so siblings will not be automatically marked
                for(const PVStructure *parent = fld->getParent(); parent; parent = parent->getParent()) {
                    mask.set(parent->getFieldOffset());
                }
            }
        }
    }
}

}
namespace epics { namespace pvData {

void printRaw(std::ostream& strm, const PVStructure::Formatter& format, const PVStructure& cur)
{
    typedef std::deque<std::pair<const PVField*, size_t> > todo_t;
    todo_t todo;
    BitSet show, highlight;

    const long orig_indent = format::indent_value(strm);

    {
        if(format.xshow)
            show = *format.xshow;
        else
            show.set(0);

        if(format.xhighlight)
            highlight = *format.xhighlight;

        expandBS(format.xtop, show, true);
        expandBS(format.xtop, highlight, false);
        highlight &= show; // can't highlight hidden fields (paranoia)
    }

    if(!show.get(0)) return; // nothing to do here...

    todo.push_front(std::make_pair(&format.xtop, orig_indent));

    while(!todo.empty()) {
        todo_t::value_type cur(todo.front());
        todo.pop_front();

        format::indent_value(strm) = cur.second;

        bool hl = highlight.get(cur.first->getFieldOffset()) && format.xmode==PVStructure::Formatter::ANSI;
        if(hl)
            strm<<"\x1b[1m"; // Bold

        switch(cur.first->getField()->getType()) {
        case structure: {
            const PVStructure* str = static_cast<const PVStructure*>(cur.first);

            const PVFieldPtrArray& flds = str->getPVFields();

            for(PVFieldPtrArray::const_reverse_iterator it(flds.rbegin()), end(flds.rend()); it!=end; ++it) {
                const PVField *fld = (*it).get();
                if(!show.get(fld->getFieldOffset())) continue;

                todo.push_front(std::make_pair(fld, cur.second+1));
            }

            std::string id(cur.first->getField()->getID());

            strm<<format::indent()<<id<<' '<<cur.first->getFieldName();
            if(id=="alarm_t") {
                strm.put(' ');
                printAlarmTx(strm, *str);
            } else if(id=="time_t") {
                strm.put(' ');
                printTimeTx(strm, *str);
            } else if(id=="enum_t") {
                strm.put(' ');
                printEnumT(strm, *str, false);
            }
            strm.put('\n');
        }
            break;
        case scalar:
        case scalarArray:
            strm<<format::indent()<<cur.first->getField()->getID()<<' '<<cur.first->getFieldName()
                <<' '<<*cur.first
                <<'\n';
            break;
        case structureArray:
        case union_:
        case unionArray:
            strm<<*cur.first;
            break;
        }

        if(hl)
            strm<<"\x1b[0m"; // reset
    }

    format::indent_value(strm) = orig_indent;
}

std::ostream& operator<<(std::ostream& strm, const PVStructure::Formatter& format)
{
    if(format.xfmt==PVStructure::Formatter::JSON) {
        JSONPrintOptions opts;
        opts.multiLine = false;
        printJSON(strm, format.xtop, format.xshow ? *format.xshow : BitSet().set(0), opts);
        strm<<'\n';
        return strm;

    } else if(format.xfmt==PVStructure::Formatter::NT) {
        std::string id(format.xtop.getStructure()->getID()),
                    idprefix(id.substr(0, id.find_first_of('.')));

        // NTTable
        if(idprefix=="epics:nt/NTTable:1") {
            if(printTable(strm, format.xtop))
                return strm;
        } else {
            //NTScalar, NTScalarArray, NTEnum, or anything with '.value'

            PVField::const_shared_pointer value(format.xtop.getSubField("value"));
            if(value) {
                switch(value->getField()->getType()) {
                case scalar:
                    strm<<format::indent();
                    printTimeT(strm, format.xtop);
                    strm<<std::setprecision(6)<<*static_cast<const PVScalar*>(value.get())<<' ';
                    printAlarmT(strm, format.xtop);
                    strm<<'\n';
                    return strm;

                case scalarArray:
                    strm<<format::indent();
                    printTimeT(strm, format.xtop);
                    printAlarmT(strm, format.xtop);
                    strm<<std::setprecision(6)<<*static_cast<const PVScalarArray*>(value.get())<<'\n';
                    return strm;

                case structure:
                    if(printEnumT(strm, format.xtop, true)) {
                        strm<<'\n';
                        return strm;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
    // fall through unhandled as Raw

    PVStructure::Formatter format2(format);

    if(format2.xmode==PVStructure::Formatter::Auto)
        format2.xmode = useEscapes(strm) ? PVStructure::Formatter::ANSI : PVStructure::Formatter::Plain;

    printRaw(strm, format2, format.xtop);

    return strm;
}

static char hexdigit(char c) {
    c &= 0xf;
    if(c<9)
        return '0'+c;
    else
        return 'A'+c-10;
}

escape::~escape() {}

std::string escape::str() const
{
    std::ostringstream strm;
    strm<<(*this);
    return strm.str();
}

epicsShareFunc
std::ostream& operator<<(std::ostream& strm, const escape& Q)
{
    for(size_t pos = 0, len = Q.orig.size(); pos < len; pos++) {
        const char C = Q.orig[pos];
        char quote = '\\', next;
        // compare me with epicsStrnEscapedFromRaw()
        switch(C) {
        case '\a': next = 'a'; break;
        case '\b': next = 'b'; break;
        case '\f': next = 'f'; break;
        case '\n': next = 'n'; break;
        case '\r': next = 'r'; break;
        case '\t': next = 't'; break;
        case '\v': next = 'v'; break;
        case '\\': next = '\\'; break;
        case '\'': next = '\''; break;
        case '\"': next = '\"'; if(Q.S==escape::CSV) quote = '"'; break;
        default:
            if(!isprint(C)) {
                // print three charator escape
                strm<<"\\x"<<hexdigit(C>>4)<<hexdigit(C);
            } else {
                // literal
                strm.put(C);
            }
            continue;
        }
        // print two charactor escape
        strm.put(quote);
        strm.put(next);
    }

    return strm;
}


std::ostream& operator<<(std::ostream& strm, const maybeQuote& q)
{
    bool esc = false;
    for(size_t i=0, N=q.s.size(); i<N && !esc; i++) {
        switch(q.s[i]) {
        case '\a':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case ' ':
        case '\v':
        case '\\':
        case '\'':
        case '\"':
            esc = true;
            break;
        default:
            if(!isprint(q.s[i])) {
                esc = true;
            }
            break;
        }
    }
    if(esc) {
        strm<<'"'<<escape(q.s)<<'"';
    } else {
        strm<<q.s;
    }
    return strm;
}

}} //epics::pvData
