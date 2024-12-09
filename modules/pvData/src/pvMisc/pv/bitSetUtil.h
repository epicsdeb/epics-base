/*bitSetUtil.h*/
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 *  @author mes
 */
#ifndef BITSETUTIL_H
#define BITSETUTIL_H

#include <pv/pvData.h>
#include <pv/bitSet.h>

#include <shareLib.h>

namespace epics { namespace pvData {

/**
 * @brief Compress a bitSet.
 *
 */
class epicsShareClass BitSetUtil {
public:
    /**
     *  compress the bitSet for a pvStructure.
     *  In all subfields of a structure have been modified then
     *  the bit for the structure is set and all the subfield bits
     *  are cleared.
     *  @param bitSet this must be a valid bitSet for pvStructure.
     *  @param pvStructure the structure.
     */
    static bool compress(BitSetPtr const &bitSet,PVStructurePtr const &pvStructure);
};

}}
#endif /*BITSETUTIL_H */
