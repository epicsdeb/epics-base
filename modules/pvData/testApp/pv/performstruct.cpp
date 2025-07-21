// Attempt to qualtify the effects of de-duplication on the time need to allocate a PVStructure
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <testMain.h>
#include <epicsUnitTest.h>

#include <pv/current_function.h>
#include <pv/pvData.h>
#include <pv/standardField.h>

namespace {

namespace pvd = epics::pvData;

struct TimeIt {
    struct timespec m_start;
    double sum, sum2;
    size_t count;
    TimeIt() { reset(); }
    void reset() {
        sum = sum2 = 0.0;
        count = 0;
    }
    void start() {
        clock_gettime(CLOCK_MONOTONIC, &m_start);
    }
    void end() {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        double diff = (end.tv_sec-m_start.tv_sec) + (end.tv_nsec-m_start.tv_nsec)*1e-9;
        sum += diff;
        sum2 += diff*diff;
        count++;
    }
    void report(const char *unit ="s", double mult=1.0) const {
        double mean = sum/count;
        double mean2 = sum2/count;
        double std = sqrt(mean2 - mean*mean);
        printf("# %zu sample   %f +- %f %s\n", count, mean/mult, std/mult, unit);
    }
};

void buildMiss()
{
    testDiag("%s", CURRENT_FUNCTION);
    TimeIt record;

    pvd::FieldCreatePtr create(pvd::getFieldCreate());
    pvd::StandardFieldPtr standard(pvd::getStandardField());

    for(size_t i=0; i<1000; i++) {
        // unique name each time to (partially) defeat caching
        char buf[10];
        sprintf(buf, "field%zu", i);

        record.start();

        pvd::FieldConstPtr fld(create->createFieldBuilder()
                               ->setId(buf)
                               ->add("value", pvd::pvInt)
                               ->addNestedStructure(buf)
                                   ->add("value", pvd::pvString)
                               ->endNested()
                               ->add("display", standard->display())
                               ->createStructure());
        record.end();
    }

    record.report("us", 1e-6);
}

void buildHit()
{
    testDiag("%s", CURRENT_FUNCTION);
    TimeIt record;

    pvd::FieldCreatePtr create(pvd::getFieldCreate());
    pvd::StandardFieldPtr standard(pvd::getStandardField());

    pvd::FieldConstPtr fld(create->createFieldBuilder()
                           ->add("value", pvd::pvInt)
                           ->addNestedStructure("foo")
                               ->add("field", pvd::pvString)
                           ->endNested()
                           ->add("display", standard->display())
                           ->createStructure());

    for(size_t i=0; i<1000; i++) {

        record.start();

        pvd::FieldConstPtr fld(create->createFieldBuilder()
                               ->add("value", pvd::pvInt)
                               ->addNestedStructure("foo")
                                   ->add("field", pvd::pvString)
                               ->endNested()
                               ->add("display", standard->display())
                               ->createStructure());
        record.end();
    }

    record.report("us", 1e-6);
}

} // namespace

MAIN(performStruct) {
    testPlan(0);
    buildMiss();
    buildHit();
    return testDone();
}
