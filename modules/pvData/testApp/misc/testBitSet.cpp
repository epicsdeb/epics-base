/* testBitSet.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Matej Sekoranja Date: 2010.10.18 */

#include <iostream>
#include <iomanip>
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <algorithm>

#include <dbDefs.h>

#include <pv/bitSet.h>
#include <pv/serializeHelper.h>
#include <pv/pvUnitTest.h>

#include <epicsUnitTest.h>
#include <testMain.h>

namespace {

using namespace epics::pvData;
using std::string;

static string toString(BitSet& bitSet)
{
    std::ostringstream oss;
    oss << bitSet;
    return oss.str();
}

void testInitialize()
{
    testDiag("testInitialize()");
#if __cplusplus>=201103L
    testOk1(BitSet().size()==0);
    testOk1(BitSet({}).size()==0);
    testEqual(BitSet().set(1).set(5).set(500), BitSet({1, 5, 500}));
#else
    testSkip(3, "Not c++11");
#endif
}

static void testGetSetClearFlip()
{
    testDiag("testGetSetClearFlip... ");

    // empty
    BitSet* b1 = new BitSet();
    testOk1(b1->isEmpty());
    testOk1(b1->cardinality() == 0);
    // to string check
    string str = toString(*b1);
    testOk1(str == "{}");

    // one
    b1->set(3);
    testOk1(b1->get(3));
    testOk1(!b1->isEmpty());
    testOk1(b1->cardinality() == 1);
    // to string check
    str = toString(*b1);
    testOk1(str == "{3}");

    // grow
    b1->set(66);
    b1->set(67);
    b1->set(68);
    testOk1(b1->cardinality() == 4);
    str = toString(*b1);
    testOk1(str == "{3, 66, 67, 68}");

    // clear one
    b1->clear(67);
    testOk1(b1->cardinality() == 3);
    str = toString(*b1);
    testOk1(str == "{3, 66, 68}");

    // flip
    b1->flip(66);
    b1->flip(130);
    testOk1(b1->cardinality() == 3);
    str = toString(*b1);
    testOk1(str == "{3, 68, 130}");

    // flip
    b1->set(130, false);
    b1->set(4, true);
    testOk1(b1->cardinality() == 3);
    str = toString(*b1);
    testOk1(str == "{3, 4, 68}");

    // clear all
    b1->clear();
    testOk1(b1->isEmpty());
    testOk1(b1->cardinality() == 0);
    str = toString(*b1);
    testOk1(str == "{}");

    delete b1;
}

static void testOperators()
{
    testDiag("testOperators... ");

    BitSet b1;
    testOk1(b1 == b1);
    BitSet b2;
    testOk1(b1 == b2);

    b1.set(1);
    testOk1(!(b1 == b2));

    testDiag("different internal length, but the same");
    b2.set(100);
    b2.set(1);
    b2.flip(100);
    testOk1(b1 == b2);

    testDiag("OR test");
    b2.set(65);
    b2.set(106);
    b2.set(105);
    b1 |= b2;
    string str = toString(b1);
    testOk1(str == "{1, 65, 105, 106}");
    b1.clear();
    b1 |= b2;
    str = toString(b1);
    testOk1(str == "{1, 65, 105, 106}");

    testDiag("AND test");
    b1.set(128);
    b1 &= b2;
    testOk1(b1 == b2);

    testDiag("XOR test");
    b1.set(128);
    b1 ^= b2;
    testOk1((b1.cardinality() == 1 && b1.get(128) == true));
    b1.clear();
    b2.clear();
    b1.set(1);
    b2.set(256);
    b1 ^= b2;
    testOk1((b1.cardinality() == 2 && b1.get(1) == true && b1.get(256) == true));


    testDiag("assign");
    testOk1(b1 != b2);
    b1 = b2;
    testOk1(b1 == b2);

    testDiag("or_and");
    b1.clear(); b1.set(2);
    b2.clear(); b2.set(66); b2.set(128);
    BitSet b3; b3.set(128); b3.set(520);
    b1.or_and(b2, b3);
    str = toString(b1);
    testOk1(str == "{2, 128}");

    b1.clear(); b1.set(1);
    b2.clear();
    b3.clear(); b3.set(1);
    std::cout<<"# "<<toString(b3)<<" |= "<<toString(b1)<<" & "<<toString(b2)<<"\n";
    b3.or_and(b1, b2);
    testOk(toString(b3) == "{1}", "%s == {1}", toString(b3).c_str());
}

static void testLogical()
{
    BitSet A, B;

    testOk1(!A.logical_and(B));
    testOk1(!A.logical_or(B));

    A.set(41);

    testOk1(!A.logical_and(B));
    testOk1(A.logical_or(B));

    A.set(42);

    testOk1(!A.logical_and(B));
    testOk1(A.logical_or(B));

    B.set(41);

    testOk1(A.logical_and(B));
    testOk1(A.logical_or(B));
}

static void tofrostring(const BitSet& in, const char *expect, size_t elen, int byteOrder)
{
    {
        std::vector<epicsUInt8> buf;
        serializeToVector(&in, byteOrder, buf);

        std::ostringstream astrm, estrm;

        bool match = buf.size()==elen;
        if(!match) testDiag("Lengths differ %u != %u", (unsigned)buf.size(), (unsigned)elen);
        for(size_t i=0, e=std::min(elen, buf.size()); i<e; i++) {
            astrm<<" "<<std::hex<<std::setfill('0')<<std::setw(2)<<int(buf[i]&0xff);
            estrm<<" "<<std::hex<<std::setfill('0')<<std::setw(2)<<int(expect[i]&0xff);
            match &= (buf[i]&0xff)==(expect[i]&0xff);
        }

        testDiag("expect %s", estrm.str().c_str());
        testDiag("actual %s", astrm.str().c_str());
        testOk(match, "Serialization %s", (byteOrder==EPICS_ENDIAN_BIG)?"BIG":"LITTLE");
    }

    {
        BitSet other;
        ByteBuffer ebuf((char*)expect, elen, byteOrder);
        try{
            deserializeFromBuffer(&other, ebuf);
        }catch(std::exception& e){
            testFail("Exception during deserialization");
            testSkip(1, "deserialize failed");
            return;
        }
        testOk(ebuf.getRemaining()==0, "buffer remaining 0 == %u", (unsigned)ebuf.getRemaining());

        std::ostringstream astrm, estrm;
        astrm << other;
        estrm << in;

        testOk(other==in,"%s == %s", astrm.str().c_str(), estrm.str().c_str());
    }
}

static void testSerialize()
{
    testDiag("testSerialization");

#define TOFRO(BB, BES, LES) do{tofrostring(BB, BES, NELEMENTS(LES)-1, EPICS_ENDIAN_BIG); \
    tofrostring(BB, LES, NELEMENTS(LES)-1, EPICS_ENDIAN_LITTLE);}while(0)
    {
        BitSet dut;
        TOFRO(dut, "\x00",
                   "\x00"); // zero size
    }
    {
        BitSet dut;
        dut.set(0);
        TOFRO(dut, "\x01\x01",
                   "\x01\x01");
    }
    {
        BitSet dut;
        dut.set(1);
        TOFRO(dut, "\x01\x02",
                   "\x01\x02");
    }
    {
        BitSet dut;
        dut.set(8);
        TOFRO(dut, "\x02\x00\x01",
                   "\x02\x00\x01"); // high 64-bit word always LSB
    }
    {
        BitSet dut;
        dut.set(55);
        dut.set(1);
        TOFRO(dut, "\x07\x02\x00\x00\x00\x00\x00\x80",
                   "\x07\x02\x00\x00\x00\x00\x00\x80");
    }
    {
        BitSet dut;
        dut.set(63);
        dut.set(1);
        TOFRO(dut, "\x08\x80\x00\x00\x00\x00\x00\x00\x02",
                   "\x08\x02\x00\x00\x00\x00\x00\x00\x80");
    }
    {
        BitSet dut;
        dut.set(64);
        dut.set(63);
        dut.set(8);
        dut.set(1);
        TOFRO(dut, "\x09\x80\x00\x00\x00\x00\x00\x01\x02\x01",
                   "\x09\x02\x01\x00\x00\x00\x00\x00\x80\x01");
    }
    {
        BitSet dut;
        dut.set(126);
        dut.set(64);
        dut.set(63);
        dut.set(1);
        TOFRO(dut, "\x10\x80\x00\x00\x00\x00\x00\x00\x02\x40\x00\x00\x00\x00\x00\x00\x01",
                   "\x10\x02\x00\x00\x00\x00\x00\x00\x80\x01\x00\x00\x00\x00\x00\x00\x40");
    }
#undef TOFRO
}

} // namespace

MAIN(testBitSet)
{
    testPlan(90);
    testInitialize();
    testGetSetClearFlip();
    testOperators();
    testLogical();
    testSerialize();
    return testDone();
}
