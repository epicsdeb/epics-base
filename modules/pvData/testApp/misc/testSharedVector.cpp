/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Michael Davidsaver */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <vector>

#include <pv/pvUnitTest.h>
#include <testMain.h>

#include "pv/sharedVector.h"

namespace {

namespace pvd = epics::pvData;

void testEmpty()
{
    testDiag("Test empty vector");
    pvd::shared_vector<pvd::int32> empty, empty2, empty3(0u);

    testOk1(empty.size()==0);
    testOk1(empty.empty());
    testOk1(empty.begin()==empty.end());
    testOk1(empty.unique());

    testOk1(empty3.empty());
    testOk1(empty3.unique());

    testOk1(empty.data()==NULL);

    testOk1(empty==empty);
    testOk1(!(empty!=empty));
    testOk1(empty==empty2);
    testOk1(!(empty!=empty2));
}

void testInternalAlloc()
{
    testDiag("Test vector alloc w/ new[]");

    pvd::shared_vector<pvd::int32> internal(5);

    testOk1(internal.size()==5);
    testOk1(!internal.empty());
    testOk1(internal.unique());
    testOk1(internal.data()!=NULL);

    testOk1(internal.begin()+5==internal.end());
    testOk1(internal.begin()==internal.end()-5);
    testOk1(internal.begin()+2==internal.end()-3);
    testOk1(internal.end()-internal.begin()==5);

    internal[2] = 42;
    testOk1(internal[2]==42);

    pvd::shared_vector<pvd::int32> internal2(15, 500);

    testOk1(internal2.size()==15);
    testOk1(internal2[1]==500);

    internal.swap(internal2);

    testOk1(internal2.size()==5);
    testOk1(internal.size()==15);

    internal.clear();

    testOk1(internal.size()==0);
    testOk1(internal.empty());
    testOk1(internal.unique());
    testOk1(internal.data()==NULL);
}

//Note: STL shared_ptr requires that deletors be copy constructable
template<typename E>
struct callCounter {
    std::tr1::shared_ptr<pvd::int32> count;
    callCounter():count(new pvd::int32){*count=0;}
    callCounter(const callCounter& o):count(o.count) {}
    callCounter& operator=(const callCounter& o){count=o.count;}
    void operator()(E){(*count)++;}
};

void testExternalAlloc()
{
    testDiag("Test vector external alloc");

    // Simulate a failed malloc() or similar
    pvd::int32 *oops=0;
    pvd::shared_vector<pvd::int32> nullPtr(oops, 42, 100);

    testOk1(nullPtr.size()==0);
    testOk1(nullPtr.empty());
    testOk1(nullPtr.begin()==nullPtr.end());
    testOk1(nullPtr.unique());

    testOk1(nullPtr.data()==NULL);

    pvd::int32 *raw=new pvd::int32[5];
    pvd::shared_vector<pvd::int32> newData(raw, 1, 4);

    testOk1(newData.size()==4);
    testOk1(!newData.empty());

    // check that offset is used
    raw[1]=14;
    testOk1(newData[0]==14);

    // Check use of custom deleter
    pvd::int32 localVar[4] = {1,2,3,4};
    callCounter<pvd::int32*> tracker;
    testOk1(*tracker.count==0);

    pvd::shared_vector<pvd::int32> locvar(localVar,
                                             tracker,
                                             0, 4);

    testOk1(locvar[1]==2);
    testOk1(*tracker.count==0);

    newData.swap(locvar);

    testOk1(*tracker.count==0);

    newData.clear();

    testOk1(*tracker.count==1);
}

void testShare()
{
    testDiag("Test vector Sharing");

    pvd::shared_vector<pvd::int32> one, two(15);
    pvd::shared_vector<pvd::int32> three(two);

    testOk1(one.unique());
    testOk1(!two.unique());
    testOk1(!three.unique());

    testOk1(two.data() == three.data());

    one = two;

    testOk1(!one.unique());

    testOk1(two.data() == one.data());

    one = one; // no-op

    testOk1(two.data() == one.data());

    one[1] = 43;
    testOk1(two[1]==43);
    testOk1(three[1]==43);

    one.make_unique();

    testOk1(one[1]==43);
    one[1] = 143;
    testOk1(two[1]==43);
    testOk1(three[1]==43);

    two.resize(two.size());

    testOk1(two[1]==43);
    two[1] = 243;
    testOk1(one[1]==143);
    testOk1(three[1]==43);

    testOk1(one.unique());
    testOk1(two.unique());
    testOk1(three.unique());

    one.resize(2);

    testOk1(one.size()==2);
    testOk1(one[1]==143);
    testOk1(two.size()==15);
    testOk1(three.size()==15);

    two.resize(20, 5000);

    testOk1(two[1]==243);
    testOk1(one.size()==2);
    testOk1(two.size()==20);
    testOk1(three.size()==15);

    testOk1(two[19]==5000);
}

void testConst()
{
    testDiag("Test constant vector");

    pvd::shared_vector<pvd::int32> writable(15, 100);

    pvd::shared_vector<pvd::int32>::reference wr = writable[0];
    pvd::shared_vector<pvd::int32>::const_reference ror = writable[0];

    testOk1(wr==ror);

    pvd::int32 *compare = writable.data();

    testOk1(writable.unique());

    // can re-target container, but data is R/O
    pvd::shared_vector<const pvd::int32> rodata(freeze(writable));

    pvd::shared_vector<const pvd::int32>::reference wcr = rodata[0];
    pvd::shared_vector<const pvd::int32>::const_reference rocr = rodata[0];

    testOk1(wcr==rocr);

    testOk1(rodata.data()==compare);
    writable = thaw(rodata);

    testOk1(writable.data()==compare);

    rodata = freeze(writable);

    testOk1(rodata.data()==compare);

    pvd::shared_vector<const pvd::int32> rodata2(rodata);

    testOk1(rodata.data()==rodata2.data());

    rodata2.make_unique();

    testOk1(rodata.data()!=rodata2.data());
}

void testSlice()
{
    testDiag("Test vector slicing");

    pvd::shared_vector<pvd::int32> original(10, 100);

    pvd::shared_vector<pvd::int32> half1(original), half2(original), half2a(original);

    half1.slice(0, 5);
    half2.slice(5, 5);
    half2a.slice(5);

    testOk1(half1.dataOffset()==0);
    testOk1(half2.dataOffset()==5);
    testOk1(half2a.dataOffset()==5);

    testOk1(half1.size()==5);
    testOk1(half2.size()==5);
    testOk1(half2a.size()==5);

    testOk1(half1.dataTotal()==10);
    testOk1(half2.dataTotal()==5);
    testOk1(half2a.dataTotal()==5);

    testOk1(original.data() == half1.data());
    testOk1(half2.data() == half2a.data());

    half1.slice(100000);
    half2.slice(1);
    half2a.slice(1,1);

    testOk1(half1.dataOffset()==5);
    testOk1(half2.dataOffset()==6);
    testOk1(half2a.dataOffset()==6);

    testOk1(half1.size()==0);
    testOk1(half2.size()==4);
    testOk1(half2a.size()==1);

    testOk1(half1.dataTotal()==5);
    testOk1(half2.dataTotal()==4);
    testOk1(half2a.dataTotal()==4);

    half2.clear();
    testOk1(half2.dataOffset()==0);
    testOk1(half2.dataCount()==0);
    testOk1(half2.dataTotal()==0);
    testOk1(half2.data()==NULL);
}

void testCapacity()
{
    testDiag("Test vector capacity");

    pvd::shared_vector<pvd::int32> vect(10, 100);

    pvd::int32 *peek = vect.dataPtr().get();

    vect.slice(0, 5);

    testOk1(vect.size()==5);
    testOk1(vect.dataTotal()==10);
    testOk1(vect.dataPtr().get() == peek);

    vect.resize(6);

    testOk1(vect.dataPtr().get() == peek);
    testOk1(vect.size()==6);
    testOk1(vect.dataTotal()==10);

    vect.resize(10);

    testOk1(vect.dataPtr().get() == peek);
    testOk1(vect.size()==10);
    testOk1(vect.dataTotal()==10);

    vect.resize(11);

    testOk1(vect.dataPtr().get() != peek);
    testOk1(vect.size()==11);
    testOk1(vect.dataTotal()>=11);

    vect[1] = 124;

    vect.reserve(15);

    testOk1(vect.size()==11);
    testOk1(vect.dataTotal()>=15);

    testOk1(vect[1]==124);
}

void testPush()
{
    pvd::shared_vector<pvd::int32> vect;

    testDiag("Test push_back optimizations");

    size_t nallocs = 0;
    size_t cap = vect.capacity();

    for(size_t s=0; s<16*1024; s++) {
        vect.push_back((int)s);

        if(cap!=vect.capacity()) {
            nallocs++;
            cap = vect.capacity();
        }
    }

    testDiag("push_back %ld times caused %ld re-allocations",
             (unsigned long)vect.size(),
             (unsigned long)nallocs);

    testOk1(nallocs==26);
}

void testVoid()
{
    testDiag("Test vector cast to/from void");

    pvd::shared_vector<pvd::int32> IV(4);

    pvd::shared_vector<void> VV(pvd::static_shared_vector_cast<void>(IV));

    testOk1(IV.dataPtr().get()==VV.dataPtr().get());
    testOk1(IV.size()*sizeof(pvd::int32)==VV.size());

    VV.slice(sizeof(pvd::int32), 2*sizeof(pvd::int32));

    IV = pvd::static_shared_vector_cast<pvd::int32>(VV);

    testOk1(IV.dataOffset()==1);
    testOk1(IV.size()==2);
    VV.clear();
}

void testConstVoid()
{
    testDiag("Test vector cast to/from const void");

    pvd::shared_vector<const pvd::int32> CIV(4);

    pvd::shared_vector<const void> CVV(pvd::static_shared_vector_cast<const void>(CIV));
    // case const void to const void
    pvd::shared_vector<const void> CVV2(pvd::static_shared_vector_cast<const void>(CVV));

    testOk1(CIV.dataPtr().get()==CVV2.dataPtr().get());
    testOk1(CIV.size()*sizeof(int)==CVV2.size());

    CVV2.slice(sizeof(int), 2*sizeof(int));

    CIV = pvd::static_shared_vector_cast<const pvd::int32>(CVV2);

    testOk1(CIV.dataOffset()==1);
    testOk1(CIV.size()==2);

    pvd::shared_vector<void> VV;
    // not possible to thaw() void as shared_vector<void> has no make_unique()
    //VV = thaw(CVV);
    CVV = freeze(VV);
}

struct dummyStruct {};

void testNonPOD()
{
    testDiag("Test vector of non-POD types");

    pvd::shared_vector<std::string> strings(6);
    pvd::shared_vector<std::tr1::shared_ptr<dummyStruct> > structs(5);

    testOk1(strings[0].empty());
    testOk1(structs[0].get()==NULL);

    structs[1].reset(new dummyStruct);
    dummyStruct *temp = structs[1].get();

    pvd::shared_vector<std::tr1::shared_ptr<dummyStruct> > structs2(structs);

    testOk1(!structs.unique());
    testOk1(structs[1].unique());

    testOk1(structs2[1].get()==temp);

    structs2.make_unique();

    testOk1(structs.unique());
    testOk1(!structs[1].unique());

    testOk1(structs2[1].get()==temp);
}

void testVectorConvert()
{
    testDiag("Test shared_vector_convert");

    pvd::shared_vector<pvd::int32> ints(6, 42), moreints;
    pvd::shared_vector<float> floats;
    pvd::shared_vector<std::string> strings;
    pvd::shared_vector<void> voids;

    testOk1(ints.unique());

    // no-op convert.  Just returns another reference
    moreints = pvd::shared_vector_convert<pvd::int32>(ints);

    testOk1(!ints.unique());
    moreints.clear();

    // conversion when both types are known.
    // returns a new vector
    floats = pvd::shared_vector_convert<float>(ints);

    testOk1(ints.unique());
    testOk1(floats.size()==ints.size());
    testOk1(floats.at(0)==42.0);

    // convert to void is static_shared_vector_cast<void>()
    // returns a reference
    voids = pvd::shared_vector_convert<void>(ints);

    testOk1(!ints.unique());
    testOk1(voids.size()==ints.size()*sizeof(pvd::int32));

    // convert from void uses shared_vector<void>::original_type()
    // to find that the actual type is 'int32'.
    // returns a new vector
    testOk1(voids.original_type()==pvd::pvInt);
    strings = pvd::shared_vector_convert<std::string>(voids);

    voids.clear();

    testOk1(ints.unique());
    testOk1(strings.size()==ints.size());
    testOk1(strings.at(0)=="42");

    testDiag("convert empty array");
    testOk1(pvd::shared_vector_convert<double>(pvd::shared_vector<pvd::int32>()).empty());
}

void testWeak()
{
    testDiag("Test weak_ptr counting");

    pvd::shared_vector<pvd::int32> data(6);

    testOk1(data.unique());

    std::tr1::shared_ptr<pvd::int32> pdata(data.dataPtr());

    testOk1(!data.unique());

    pdata.reset();

    testOk1(data.unique());

    std::tr1::weak_ptr<pvd::int32> wdata(data.dataPtr());

    testOk1(data.unique()); // True, but I wish it wasn't!!!

    pdata = wdata.lock();

    testOk1(!data.unique());
}

void testICE()
{
    testDiag("Test freeze and thaw");

    pvd::shared_vector<pvd::int32> A(6, 42), C;
    pvd::shared_vector<const pvd::int32> B, D;

    pvd::int32 *check = A.data();

    // check freeze w/ unique reference

    // clears A and moves reference to B
    // no copy
    B = pvd::freeze(A);

    testOk1(A.unique());
    testOk1(B.unique());
    testOk1(A.size()==0);
    testOk1(B.size()==6);
    testOk1(A.data()!=check);
    testOk1(B.data()==check);

    D = B; // create second const reference

    // clears D, but reference to B
    // remains, so a copy is made
    C = pvd::thaw(D);

    testOk1(B.unique());
    testOk1(C.unique());
    testOk1(B.size()==6);
    testOk1(C.size()==6);
    testOk1(B.data()==check);
    testOk1(C.data()!=NULL);
    testOk1(C.at(0)==42);

    C.clear();

    // clears B and moves reference to A
    // no copy
    A = pvd::thaw(B);

    testOk1(A.unique());
    testOk1(B.unique());
    testOk1(A.size()==6);
    testOk1(B.size()==0);
    testOk1(A.data()==check);
    testOk1(B.data()!=check);

    C = A; // create second non-const reference

    testOk1(!A.unique());

    try {
        // would clear A, but remaining reference C
        // fails operation.  A not cleared
        // and exception thrown
        B = pvd::freeze(A);
        testFail("Froze non-unique vector!");
    } catch(std::runtime_error& e) {
        testPass("freeze of non-unique throws runtime_error as expected");
    }
}

void testBad()
{
    pvd::shared_vector<int> I;
    pvd::shared_vector<const int> CI;
    pvd::shared_vector<float> F;
    pvd::shared_vector<const float> CF;
    pvd::shared_vector<void> V;
    pvd::shared_vector<const void> CV;
    (void)I;
    (void)CI;
    (void)F;
    (void)CF;
    (void)V;
    (void)CV;

    // Tests which should result in compile failure.
    // as there is no established way to test this automatically,
    // uncomment one at a time

    // No copy from const to non-const
    //CI = I;
    //I = CI;
    //pvd::shared_vector<const int> CI2(I);
    //pvd::shared_vector<int> I2(CI);

    // shared_vector_convert can't thaw()
    //I = pvd::shared_vector_convert<int>(CI);
    //V = pvd::shared_vector_convert<void>(CV);

    // shared_vector_convert can't freeze()
    //CI = pvd::shared_vector_convert<const int>(I);
    //CV = pvd::shared_vector_convert<const void>(V);

    // static_shared_vector_cast can't thaw()
    //I = pvd::static_shared_vector_cast<int>(CI);
    //V = pvd::static_shared_vector_cast<void>(CV);

    // static_shared_vector_cast can't freeze()
    //CI = pvd::static_shared_vector_cast<const int>(I);
    //CV = pvd::static_shared_vector_cast<const void>(V);

    // freeze() can't change type.
    // the error here will be with the assignment
    //I = pvd::freeze(CV);
    //I = pvd::freeze(CF);
    //CI = pvd::freeze(V);
    //CI = pvd::freeze(F);

    // that() can't change type.
    // the error here will be with the assignment
    //CI = pvd::thaw(V);
    //CI = pvd::thaw(F);
    //I = pvd::thaw(CV);
    //I = pvd::that(CF);
}

void testAutoSwap()
{
    epics::auto_ptr<int> A(new int(42)), B(new int(43));

    epics::swap(A, B);

    testOk1(43==*A);
    testOk1(42==*B);
}

void testCXX11Move()
{
#if __cplusplus>=201103L
    testDiag("Check std::move()");
    pvd::shared_vector<pvd::int32> A(4, 42),
                                   B(std::move(A));

    testOk1(A.unique());
    testOk1(B.unique());
    testOk1(A.empty());
    testOk1(B.size()==4);
    testOk1(!B.empty() && B[0]==42);

    A = std::move(B);

    testOk1(A.unique());
    testOk1(B.unique());
    testOk1(B.empty());
    testOk1(A.size()==4);
    testOk1(!A.empty() && A[0]==42);

    pvd::shared_vector<void> C(pvd::shared_vector_convert<void>(A)),
                             D(std::move(C));
    A.clear();

    testOk1(C.unique());
    testOk1(D.unique());
    testOk1(C.empty());
    testOk1(D.size()==4*4);

    C = std::move(D);

    testOk1(C.unique());
    testOk1(D.unique());
    testOk1(C.size()==4*4);
    testOk1(D.empty());
#else
    testSkip(18, "Not -std=c++11");
#endif
}

void testCXX11Init()
{
#if __cplusplus>=201103L
    testDiag("Check c++11 style array initialization");

    pvd::shared_vector<const pvd::int32> A = {1.0, 2.0, 3.0};

    testOk1(A.size()==3);

    pvd::int32 sum = 0;
    for (auto V: A) {
        sum += V;
    }
    testOk1(sum==6);
#else
    testSkip(2, "Not -std=c++11");
#endif
}

} // namespace

MAIN(testSharedVector)
{
    testPlan(192);
    testDiag("Tests for shared_vector");

    testDiag("sizeof(shared_vector<pvd::int32>)=%lu",
             (unsigned long)sizeof(pvd::shared_vector<pvd::int32>));

    testEmpty();
    testInternalAlloc();
    testExternalAlloc();
    testCapacity();
    testShare();
    testConst();
    testSlice();
    testPush();
    testVoid();
    testConstVoid();
    testNonPOD();
    testVectorConvert();
    testWeak();
    testICE();
    testBad();
    testAutoSwap();
    testCXX11Move();
    testCXX11Init();
    return testDone();
}
