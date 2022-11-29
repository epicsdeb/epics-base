/* ntutils.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTUTILS_H
#define NTUTILS_H

#include <string>
#include <shareLib.h>

namespace epics { namespace nt {

/**
 * @brief Utility methods for NT types.
 * 
 * @author mse
 */
class epicsShareClass NTUtils {
public:

    /**
     * Checks whether NT types are compatible by checking their IDs,
     * i.e. their names and major version must match.
     * @param u1 the first URI.
     * @param u2 the second URI.
     * @return true if URIs are compatible, false otherwise.
     */
    static bool is_a(const std::string &u1, const std::string &u2);

private:
    // disable object creation
    NTUtils() {}
};

}}

#endif  /* NTUTILS_H */

