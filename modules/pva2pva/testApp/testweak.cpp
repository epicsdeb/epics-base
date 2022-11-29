
#include "weakset.h"
#include "weakmap.h"

#include <epicsUnitTest.h>
#include <testMain.h>

namespace {

static
void testWeakSet1()
{
    testDiag("Test1 weak_set");

    typedef weak_set<int> set_type;
    set_type set;
    set_type::value_pointer ptr;

    testOk1(set.empty());

    ptr.reset(new int(5));
    set.insert(ptr);

    set.insert(ptr); // second insert is a no-op

    testOk1(ptr.unique()); // we hold the only "fake" strong ref.

    {
        set_type::set_type S(set.lock_set());
        testOk1(!S.empty());
        testOk1(S.size()==1);
        testOk1(S.find(ptr)!=S.end());
    }
    {
        set_type::vector_type S(set.lock_vector());
        testOk1(!S.empty());
        testOk1(S.size()==1);
        testOk1(*S[0]==5);
    }
}

static
void testWeakSet2()
{
    testDiag("Test2 weak_set");

    typedef weak_set<int> set_type;
    set_type set;
    set_type::value_pointer ptr;

    testOk1(set.empty());

    ptr.reset(new int(5));
    set.insert(ptr);

    testOk1(!set.empty());

    testOk1(ptr.unique());
    ptr.reset(); // implicitly removes from set

    testOk1(set.empty());

    ptr.reset(new int(5));
    set.insert(ptr);

    set.clear();
    testOk1(set.empty());
    testOk1(!!ptr);
}

static
void testWeakSetInvalid()
{
    testDiag("Test adding non-unique");
    weak_set<int> set;
    weak_set<int>::value_pointer ptr(new int(5)),
            other(ptr);

    testOk1(!ptr.unique());

    try{
        set.insert(ptr);
        testFail("Missed expected exception");
    } catch(std::invalid_argument& e) {
        testPass("Got expected exception: %s", e.what());
    }
}

static
void testWeakMap1()
{
    testDiag("Test weak_value_map1");

    typedef weak_value_map<int,int> map_type;
    map_type::value_pointer ptr;
    map_type map;

    testOk1(map.empty());

    ptr.reset(new int(5));
    map[4] = ptr;

    testOk1(!map.empty());
    {
        map_type::lock_vector_type V(map.lock_vector());
        testOk1(V.size()==1);
        testOk1(V[0].first==4);
        testOk1(*V[0].second==5);
    }

    testOk1(map[4]==ptr);
    testOk1(*map[4]==5);
}

static
void testWeakMap2()
{
    testDiag("Test weak_value_map2");

    typedef weak_value_map<int,int> map_type;
    map_type::value_pointer ptr;
    map_type map;

    testOk1(map.empty());

    ptr.reset(new int(5));
    map[4] = ptr;

    testOk1(!map.empty());
    {
        map_type::lock_vector_type V(map.lock_vector());
        testOk1(V.size()==1);
        testOk1(V[0].first==4);
        testOk1(*V[0].second==5);
    }

    ptr.reset();
    testOk1(map.empty());

    ptr.reset(new int(5));
    map[4] = ptr;
    {
        map_type::value_pointer O(map[4]);
        testOk1(O==ptr);
    }

    testOk1(map.size()==1);
    map.clear();
    testOk1(map.empty());
    testOk1(!!ptr);
}

static
void testWeakLock()
{
    typedef weak_value_map<int,int> map_type;
    map_type::value_pointer ptr;
    map_type map;

    {
        map_type::guard_type G(map.mutex());

        ptr.reset(new int(42));
        map[4] = ptr;
    }
}

static
void testWeakIterate()
{
    typedef weak_set<int> set_type;
    set_type::value_pointer A, B;
    set_type set;

    testDiag("Test weak_set locked iteration");

    A.reset(new int(42));
    set.insert(A);
    A.reset(new int(43));
    // ref. to 42 is dropped
    set.insert(A);
    B.reset(new int(44));
    set.insert(B);

    testOk1(set.size()==2);

    {
        set_type::iterator it(set);
        set_type::value_pointer V;

        V = it.next();
        testOk1(V && (*V==43 || *V==44));
        V = it.next();
        testOk1(V && (*V==43 || *V==44));
        V = it.next();
        testOk1(!V);
    }
}

} // namespace

MAIN(testweak)
{
    testPlan(37);
    testWeakSet1();
    testWeakSet2();
    testWeakSetInvalid();
    testWeakMap1();
    testWeakMap2();
    testWeakLock();
    testWeakIterate();
    return testDone();
}
