/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <string>
#include <vector>

#include <epicsUnitTest.h>

#define epicsExportSharedSymbols
#include <pv/pvUnitTest.h>

namespace detail {

testPassx::~testPassx() {
    if(!alive) return;
    std::string msg(strm.str());
    size_t nl = msg.find_first_of('\n');
    if(nl==msg.npos) {
        // single-line output
        if(dotest)
            testOk(pass, "%s", msg.c_str());
        else
            testDiag("%s", msg.c_str());

    } else {
        // multi-line output
        std::istringstream lines(msg);
        std::string line;
        bool first = true;
        while(std::getline(lines ,line)) {
            if(dotest && first) {
                first = false;
                testOk(pass, "%s", line.c_str());
            } else {
                testDiag("%s", line.c_str());
            }
        }
    }
}

testPassx::testPassx(testPassx& o)
    :strm(o.strm.str())
    ,dotest(o.dotest)
    ,pass(o.pass)
    ,alive(o.alive)
{
    strm.seekp(0, std::ios_base::end);
    o.alive = false;
}

} // namespace detail
