/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <sstream>
#include <vector>

#include <testMain.h>
#include <epicsString.h>

#include <pv/pvUnitTest.h>
#include <pv/current_function.h>

#include <pv/bitSet.h>
#include <pv/pvData.h>
#include <pv/standardField.h>

namespace pvd = epics::pvData;

typedef std::vector<std::string> lines_t;

namespace {

struct SB {
    std::ostringstream strm;
    operator std::string() { return strm.str(); }
    template<typename T>
    SB& operator<<(const T& v) {
        strm<<v;
        return *this;
    }
};

lines_t lines(const std::string& str)
{
    lines_t ret;
    size_t p = 0;
    while(true) {
        size_t next = str.find_first_of('\n', p);
        ret.push_back(str.substr(p, next-p)); // exclude trailing '\n'
        if(next==str.npos)
            break;
        else
            p = next+1;
    }
    return ret;
}

std::string print(const pvd::PVStructure::Formatter& fmt)
{
    std::ostringstream strm;
    strm<<fmt;
    return strm.str();
}

struct point {
    size_t L, R;
};

// really primative diff
// expect -> actual
::detail::testPassx
testDiff(const std::string& expect, const std::string& actual, const std::string& msg = std::string())
{
    bool match = expect==actual;
    ::detail::testPassx ret(match);
    ret<<msg<<'\n';

    lines_t lhs(lines(expect)), rhs(lines(actual));

    size_t L=0, R=0;

    while(L<lhs.size() && R<rhs.size()) {
        if(lhs[L]==rhs[R]) {
            ret<<"  "<<pvd::escape(lhs[L])<<'\n';
            L++;
            R++;

        } else {
            // ugly... diagonalization hardcoded...
            static const point search[] = {
                {1,0},
                {0,1},
                {1,1},
                {2,1},
                {1,2},
                {2,2},
            };


            size_t Lp, Rp;
            for(size_t n=0, N=sizeof(search)/sizeof(search[0]); n<N; n++) {
                Lp = L+search[n].L;
                Rp = R+search[n].R;

                if(Lp<lhs.size() && Rp<rhs.size() && lhs[Lp]==rhs[Rp])
                    break;
            }
            if(Lp>=lhs.size() || Rp>=rhs.size()) {
                // reached end without match
                Lp = lhs.size();
                Rp = rhs.size();
            }

            for(size_t l=L; l<Lp; l++)
                ret<<"- "<<pvd::escape(lhs[l])<<'\n';
            for(size_t r=R; r<Rp; r++)
                ret<<"+ "<<pvd::escape(rhs[r])<<'\n';
            assert(Lp>L); // must make progress
            assert(Rp>R);
            L = Lp;
            R = Rp;
            // loop around and print matching line
        }
    }

    for(; L<lhs.size(); L++)
        ret<<"- "<<pvd::escape(lhs[L])<<'\n';
    for(; R<rhs.size(); R++)
        ret<<"+ "<<pvd::escape(rhs[R])<<'\n';

    return ret;
}

static const pvd::StructureConstPtr scalarNumeric(pvd::getFieldCreate()->createFieldBuilder()
                                                  ->setId("epics:nt/NTScalar:1.0")
                                                  ->add("value", pvd::pvInt)
                                                  ->add("alarm", pvd::getStandardField()->alarm())
                                                  ->add("timeStamp", pvd::getStandardField()->timeStamp())
                                                  ->createStructure());

void showNTScalarNumeric()
{
    testDiag("%s", CURRENT_FUNCTION);
    pvd::PVStructurePtr input(pvd::getPVDataCreate()->createPVStructure(scalarNumeric));
    input->getSubFieldT<pvd::PVScalar>("value")->putFrom(pvd::int32(-42));

    testDiff("<undefined>              -42 \n", print(input->stream()));

    input->getSubFieldT<pvd::PVScalar>("alarm.severity")->putFrom(pvd::int32(1));
    input->getSubFieldT<pvd::PVScalar>("alarm.status")->putFrom(pvd::int32(1));
    input->getSubFieldT<pvd::PVString>("alarm.message")->put("FOO");

    testDiff("<undefined>              -42 MINOR DEVICE FOO \n", print(input->stream()));
}

static const pvd::StructureConstPtr scalarString(pvd::getFieldCreate()->createFieldBuilder()
                                                  ->setId("epics:nt/NTScalar:1.0")
                                                  ->add("value", pvd::pvString)
                                                  ->add("alarm", pvd::getStandardField()->alarm())
                                                  ->add("timeStamp", pvd::getStandardField()->timeStamp())
                                                  ->createStructure());

void showNTScalarString()
{
    testDiag("%s", CURRENT_FUNCTION);
    pvd::PVStructurePtr input(pvd::getPVDataCreate()->createPVStructure(scalarString));
    testDiff("<undefined>               \n", print(input->stream()));

    input->getSubFieldT<pvd::PVString>("value")->put("bar");

    testDiff("<undefined>              bar \n", print(input->stream()));

    input->getSubFieldT<pvd::PVScalar>("alarm.severity")->putFrom(pvd::int32(1));
    input->getSubFieldT<pvd::PVScalar>("alarm.status")->putFrom(pvd::int32(1));
    input->getSubFieldT<pvd::PVString>("alarm.message")->put("FOO");

    testDiff("<undefined>              bar MINOR DEVICE FOO \n", print(input->stream()));
}

static const pvd::StructureConstPtr ntenum(pvd::getFieldCreate()->createFieldBuilder()
                                                  ->setId("epics:nt/NTEnum:1.0")
                                                  ->addNestedStructure("value")
                                                     ->setId("enum_t")
                                                     ->add("index", pvd::pvInt)
                                                     ->addArray("choices", pvd::pvString)
                                                  ->endNested()
                                                  ->add("alarm", pvd::getStandardField()->alarm())
                                                  ->add("timeStamp", pvd::getStandardField()->timeStamp())
                                                  ->createStructure());

void showNTEnum()
{
    testDiag("%s", CURRENT_FUNCTION);
    pvd::PVStructurePtr input(pvd::getPVDataCreate()->createPVStructure(ntenum));
    testDiff("<undefined>              (0) <undefined>\n", print(input->stream()), "empty");

    pvd::PVStringArray::svector sarr;
    sarr.push_back("one");
    sarr.push_back("a two");
    input->getSubFieldT<pvd::PVStringArray>("value.choices")->replace(pvd::freeze(sarr));

    input->getSubFieldT<pvd::PVInt>("value.index")->put(0);

    testDiff("<undefined>              (0) one\n", print(input->stream()), "one");

    input->getSubFieldT<pvd::PVInt>("value.index")->put(1);

    testDiff("<undefined>              (1) \"a two\"\n", print(input->stream()), "two");

    testDiff("epics:nt/NTEnum:1.0 \n"
             "    enum_t value (1) \"a two\"\n"
             "        int index 1\n"
             "        string[] choices [one, \"a two\"]\n"
             "    alarm_t alarm \n"
             "        int severity 0\n"
             "        int status 0\n"
             "        string message \n"
             "    time_t timeStamp <undefined>              \n"
             "        long secondsPastEpoch 0\n"
             "        int nanoseconds 0\n"
             "        int userTag 0\n",
             print(input->stream().format(pvd::PVStructure::Formatter::Raw)), "two raw");
}

static const pvd::StructureConstPtr table(pvd::getFieldCreate()->createFieldBuilder()
                                                  ->setId("epics:nt/NTTable:1.0")
                                                  ->addArray("labels", pvd::pvString)
                                                  ->addNestedStructure("value")
                                                    ->addArray("colA", pvd::pvInt)
                                                    ->addArray("colB", pvd::pvString)
                                                  ->endNested()
                                                  ->add("alarm", pvd::getStandardField()->alarm())
                                                  ->add("timeStamp", pvd::getStandardField()->timeStamp())
                                                  ->createStructure());
void showNTTable()
{
    testDiag("%s", CURRENT_FUNCTION);
    pvd::PVStructurePtr input(pvd::getPVDataCreate()->createPVStructure(table));

    testDiff("<undefined>                \n"
             "colA colB\n"
             , print(input->stream()),
             "empty table");


    pvd::PVStringArray::svector sarr;
    sarr.push_back("labelA");
    sarr.push_back("label B");
    input->getSubFieldT<pvd::PVStringArray>("labels")->replace(pvd::freeze(sarr));

    pvd::PVIntArray::svector iarr;
    iarr.push_back(1);
    iarr.push_back(2);
    iarr.push_back(3);
    iarr.push_back(42); // will not be shown
    input->getSubFieldT<pvd::PVIntArray>("value.colA")->replace(pvd::freeze(iarr));

    sarr.push_back("one\x7f");
    sarr.push_back("two words");
    sarr.push_back("A '\"'");
    input->getSubFieldT<pvd::PVStringArray>("value.colB")->replace(pvd::freeze(sarr));


    testDiff("<undefined>                \n"
             "labelA   \"label B\"\n"
             "     1     one\\x7F\n"
             "     2 \"two words\"\n"
             "     3  \"A \\'\"\"\\'\"\n"
             , print(input->stream()),
             "with data");
}

static const pvd::StructureConstPtr everything(pvd::getFieldCreate()->createFieldBuilder()
                                               ->setId("omg")
                                               ->add("scalar", pvd::pvString)
                                               ->addArray("scalarArray", pvd::pvString)
                                               ->addNestedStructure("below")
                                                    ->add("A", pvd::pvInt)
                                                    ->addNestedUnion("select")
                                                        ->add("one", pvd::pvInt)
                                                        ->add("two", pvd::pvInt)
                                                    ->endNested()
                                                    ->addNestedUnionArray("arrselect")
                                                        ->add("foo", pvd::pvInt)
                                                        ->add("bar", pvd::pvInt)
                                                    ->endNested()
                                                    ->addNestedStructureArray("astruct")
                                                       ->add("red", pvd::pvInt)
                                                       ->add("blue", pvd::pvInt)
                                                   ->endNested()
                                               ->endNested()
                                               ->add("anything", pvd::getFieldCreate()->createVariantUnion())
                                               ->add("arrayany", pvd::getFieldCreate()->createVariantUnionArray())
                                               ->createStructure());

void testRaw()
{
    testDiag("%s", CURRENT_FUNCTION);
    pvd::PVStructurePtr input(pvd::getPVDataCreate()->createPVStructure(everything));

    {
        pvd::PVStringArray::svector temp;
        temp.push_back("hello");
        temp.push_back("world\x7f");
        input->getSubFieldT<pvd::PVStringArray>("scalarArray")->replace(pvd::freeze(temp));
    }

    testDiff("omg \n"
             "    string scalar \n"          // bit 1
             "    string[] scalarArray [hello, \"world\\x7F\"]\n"
             "    structure below\n"
             "        int A 0\n"             // bit 4
             "        union select\n"
             "            (none)\n"
             "        union[] arrselect\n"
             "        structure[] astruct\n"
             "    any anything\n"
             "        (none)\n"
             "    any[] arrayany\n"
             , print(input->stream()));

    testDiff("omg \n"
             "    string scalar \n"
             "    structure below\n"
             "        int A 0\n"
             , print(input->stream().show(pvd::BitSet().set(1).set(4))));

    testDiff("omg \n"
             "\033[1m    string scalar \n"
             "\033[0m\033[1m    string[] scalarArray [hello, \"world\\x7F\"]\n"
             "\033[0m    structure below\n"
             "\033[1m        int A 0\n"
             "\033[0m        union select\n"
             "            (none)\n"
             "        union[] arrselect\n"
             "        structure[] astruct\n"
             "    any anything\n"
             "        (none)\n"
             "    any[] arrayany\n"
             , print(input->stream()
                     .mode(pvd::PVStructure::Formatter::ANSI) // force use of escapes
                     .highlight(pvd::BitSet().set(1).set(2).set(4))
                     ));
}

void testEscape()
{
    testDiag("%s", CURRENT_FUNCTION);

    testEqual("hello world", std::string(SB()<<pvd::escape("hello world")));
    testEqual("hello\\nworld", std::string(SB()<<pvd::escape("hello\nworld")));
    testEqual("hello\\\"world", std::string(SB()<<pvd::escape("hello\"world")));
    testEqual("hello\\x7Fworld", std::string(SB()<<pvd::escape("hello\x7Fworld")));

    testEqual("hello\"\"world", std::string(SB()<<pvd::escape("hello\"world").style(pvd::escape::CSV)));

    testEqual("hello\"\"world", pvd::escape("hello\"world").style(pvd::escape::CSV).str());

    testEqual("hello_world", std::string(SB()<<pvd::maybeQuote("hello_world")));
    testEqual("\"hello_world\\\"\"", std::string(SB()<<pvd::maybeQuote("hello_world\"")));
    testEqual("\"hello world\"", std::string(SB()<<pvd::maybeQuote("hello world")));
    testEqual("\"hello\\nworld\"", std::string(SB()<<pvd::maybeQuote("hello\nworld")));
    testEqual("\"hello\\\"world\"", std::string(SB()<<pvd::maybeQuote("hello\"world")));
    testEqual("\"hello\\x7Fworld\"", std::string(SB()<<pvd::maybeQuote("hello\x7Fworld")));
}

} // namespace

MAIN(testprinter)
{
    testPlan(26);
    showNTScalarNumeric();
    showNTScalarString();
    showNTEnum();
    showNTTable();
    testRaw();
    testEscape();
    return testDone();
}
