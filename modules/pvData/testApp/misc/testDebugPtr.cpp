/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/* Author:  Michael Davidsaver */

#include <iostream>
#include <sstream>
#include <exception>
#include <ostream>
#include <iterator>

#include <epicsUnitTest.h>
#include <testMain.h>

#if __cplusplus>=201103L

#include <pv/debugPtr.h>

namespace {
void show(const epics::debug::shared_ptr_base& ref)
{
    epics::debug::ptr_base::ref_set_t refs;
    ref.spy_refs(refs);
    std::ostringstream strm;
    std::copy(refs.begin(), refs.end(), std::ostream_iterator<const void*>(strm, ", "));
    testDiag("refs: %s", strm.str().c_str());
    ref.show_refs(std::cout);
}

void testEmpty()
{
    testDiag("testEmpty()");

    epics::debug::shared_ptr<int> empty;
    testOk1(!empty);

    epics::debug::ptr_base::ref_set_t refs;
    empty.spy_refs(refs);
    testOk1(refs.empty());
}

void testSimple()
{
    testDiag("testSimple()");

    testDiag("ctor");
    epics::debug::shared_ptr<int> one(new int(42)),
                           two;
    testDiag("one = %p  two = %p", &one, &two);
    show(one);
    show(two);
    {
        epics::debug::ptr_base::ref_set_t refs;
        one.spy_refs(refs);
        testOk1(refs.size()==1);
        testOk1(refs.find(&one)!=refs.end());
        testOk1(refs.find(&two)==refs.end());
    }

    testDiag("assign non-NULL");
    two = one;
    show(one);
    show(two);
    {
        epics::debug::ptr_base::ref_set_t refs;
        one.spy_refs(refs);
        testOk1(refs.size()==2);
        testOk1(refs.find(&one)!=refs.end());
        testOk1(refs.find(&two)!=refs.end());
    }

    testDiag("reset");
    one.reset();
    show(one);
    show(two);
    {
        epics::debug::ptr_base::ref_set_t refs;
        one.spy_refs(refs);
        testOk1(refs.size()==0);
    }
    {
        epics::debug::ptr_base::ref_set_t refs;
        two.spy_refs(refs);
        testOk1(refs.size()==1);
        testOk1(refs.find(&one)==refs.end());
        testOk1(refs.find(&two)!=refs.end());
    }

    testDiag("copy ctor");
    epics::debug::shared_ptr<int> three(two),
                           empty(one);
    show(three);
    show(empty);
    testDiag("three = %p  empty = %p", &three, &empty);
    {
        epics::debug::ptr_base::ref_set_t refs;
        empty.spy_refs(refs);
        testOk1(refs.size()==0);
    }
    {
        epics::debug::ptr_base::ref_set_t refs;
        three.spy_refs(refs);
        testOk1(refs.size()==2);
        testOk1(refs.find(&one)==refs.end());
        testOk1(refs.find(&two)!=refs.end());
        testOk1(refs.find(&three)!=refs.end());
        testOk1(refs.find(&empty)==refs.end());
    }
}

struct Base {virtual ~Base(){}};
struct Derv : public Base {virtual ~Derv(){}};

void testCast()
{
    testDiag("testCast()");

    epics::debug::shared_ptr<int> one(new int(42));
    testOk1(!!one);

    epics::debug::shared_ptr<void> two(one);
    testOk1(one.get()==two.get());

    epics::debug::shared_ptr<const int> three(one);
    testOk1(one.get()==three.get());

    epics::debug::shared_ptr<int> four(epics::debug::const_pointer_cast<int>(three));
    testOk1(one.get()==four.get());

    epics::debug::shared_ptr<Base> X(new Derv);
    epics::debug::shared_ptr<Derv> Y(epics::debug::static_pointer_cast<Derv>(X));
    testOk1(X.get()==Y.get());
}

void testWeak()
{
    testDiag("testWeak()");

    epics::debug::shared_ptr<int> one(new int(42));
    {
        epics::debug::weak_ptr<int> two(one);
        epics::debug::shared_ptr<int> three(two);
        testOk1(three.get()==one.get());
    }

    {
        epics::debug::weak_ptr<int> two(one);
        epics::debug::shared_ptr<int> three(two.lock());
        testOk1(three.get()==one.get());
    }
}

struct MySelf : public epics::debug::enable_shared_from_this<MySelf>
{};

void testEnable()
{
    testDiag("testEnable()");

    epics::debug::shared_ptr<MySelf> self(new MySelf),
                              other(self->shared_from_this());

    testOk1(!!self);
    testOk1(self.get()==other.get());
}

template<typename T>
struct set_flag_dtor {
    bool *pflag;
    set_flag_dtor(bool *pflag) :pflag(pflag) {}
    void operator()(T* x) {
        delete x;
        *pflag = true;
        pflag = 0; // paranoia
    }
};

void testDtor()
{
    testDiag("testDtor()");

    bool flag = false;
    {
        epics::debug::shared_ptr<Derv> x(new Derv, set_flag_dtor<Derv>(&flag));
        testOk1(!!x);
        testOk1(!flag);
    }
    testOk1(flag);

    flag = false;
    bool flag2 = false;
    {
        epics::debug::shared_ptr<Derv> x(new Derv, set_flag_dtor<Derv>(&flag));
        testOk1(!!x);
        Derv *old = x.get();
        testOk1(!flag);
        testOk1(!flag2);
        x.reset(new Derv, set_flag_dtor<Derv>(&flag2));
        testOk1(!!x);
        testOk1(old!=x.get());
        testOk1(flag);
        testOk1(!flag2);
    }
    testOk1(flag2);

    testDiag("destroy Derv as Base");
    flag = false;
    {
        epics::debug::shared_ptr<Base> x(new Derv, set_flag_dtor<Base>(&flag));
        testOk1(!!x);
        testOk1(!flag);
    }
    testOk1(flag);
}

struct BaseEnable {
    virtual ~BaseEnable() {}
    virtual epics::debug::shared_ptr<BaseEnable> self() =0;
};

struct DervEnable : public BaseEnable,
                    public epics::debug::enable_shared_from_this<DervEnable>
{
    virtual ~DervEnable() {}
    virtual epics::debug::shared_ptr<BaseEnable> self() {
        return shared_from_this();
    }
};

void testEnableDerv()
{
    testDiag("testEnableDerv()");
    epics::debug::shared_ptr<BaseEnable> x(new DervEnable),
                                  y(x->self());
    testOk1(!!x);
    testOk1(x.get()==y.get());

    BaseEnable *old = x.get();

    x.reset();
    testOk1(!x);
    testOk1(y.unique());

    x.reset(new DervEnable);
    y = x->self();
    testOk1(!!x);
    testOk1(x.get()!=old);
    testOk1(x.get()==y.get());
}

} // namespace

MAIN(testDebugPtr)
{
    testPlan(48);
    try {
        testEmpty();
        testSimple();
        testCast();
        testWeak();
        testEnable();
        testDtor();
        testEnableDerv();
    }catch(std::exception& e){
        testAbort("Unhandled exception: %s", e.what());
    }
    return testDone();
}

#else // __cplusplus>=201103L

MAIN(testDebugPtr)
{
    testPlan(1);
    testSkip(1, "Not c++11");
    return testDone();
}

#endif // __cplusplus>=201103L
