/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#ifndef PVSUPPORT_H
#define PVSUPPORT_H

#include <list>
#include <map>

#include <pv/pvData.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PVSupport;
typedef std::tr1::shared_ptr<PVSupport> PVSupportPtr;

/**
 * @brief Base interface for a PVSupport.
 *
 */
class epicsShareClass PVSupport
{
public:
    POINTER_DEFINITIONS(PVSupport);
    /**
     * The Destructor.
     */
    virtual ~PVSupport(){}
    /**
     * @brief Optional  initialization method.
     *
     * Called after PVRecord is created but before record is installed into PVDatabase.
     *
     * @param pvValue The field to support.
     * @param pvSupport Support specific fields.
     * @return <b>true</b> for success and <b>false</b> for failure.
     */
    virtual bool init(
        epics::pvData::PVFieldPtr const & pvValue,
        epics::pvData::PVFieldPtr const & pvSupport) {return true;}
    /**
     *  @brief Optional method for derived class.
     *
     * It is called before record is added to database.
     */
    virtual void start() {}
    /**
     * @brief Virtual method for derived class.
     *
     * Called when record is processed.
     *  It is the method that implements support.
     *  It is called each time the record is processed.
     *
     * @return Returns true is any fields were modified; otherwise false.
     */
    virtual bool process() = 0;
    /**
     *  @brief Optional method for derived class.
     *
     */
    virtual void reset() {};
};

}}

#endif  /* PVSUPPORT_H */
