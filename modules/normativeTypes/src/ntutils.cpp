/* ntutils.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#define epicsExportSharedSymbols
#include <pv/ntutils.h>

using namespace std;

namespace epics { namespace nt {

bool NTUtils::is_a(const std::string &u1, const std::string &u2)
{
    // remove minor for the u1
    size_t pos1 = u1.find_last_of('.');
    std::string su1 = (pos1 == string::npos) ? u1 : u1.substr(0, pos1);

    // remove minor for the u2
    size_t pos2 = u2.find_last_of('.');
    std::string su2 = (pos2 == string::npos) ? u2 : u2.substr(0, pos2);

    return su2 == su1;
}

}}
