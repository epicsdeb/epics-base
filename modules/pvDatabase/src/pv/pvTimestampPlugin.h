/* pvTimeStampPlugin.h */
/*
 * The License for this software can be found in the file LICENSE that is included with the distribution.
 */

#ifndef PVTIMESTAMPPLUGIN_H
#define PVTIMESTAMPPLUGIN_H

#include <string>
#include <map>
#include <pv/lock.h>
#include <pv/pvData.h>
#include <pv/pvPlugin.h>
#include <pv/pvTimeStamp.h>

#include <shareLib.h>

namespace epics { namespace pvCopy{

class PVTimestampPlugin;
class PVTimestampFilter;

typedef std::tr1::shared_ptr<PVTimestampPlugin> PVTimestampPluginPtr;
typedef std::tr1::shared_ptr<PVTimestampFilter> PVTimestampFilterPtr;


/**
 * @brief  A plugin for a filter that sets a timeStamp to the current time.
 *
 * @author mrk
 * @since date 2017.03.24
 */
class epicsShareClass PVTimestampPlugin : public PVPlugin
{
private:
    PVTimestampPlugin();
public:
    POINTER_DEFINITIONS(PVTimestampPlugin);
    virtual ~PVTimestampPlugin();
    /**
     * Factory
     */
    static void create();
    /**
     * Create a PVFilter.
     * @param requestValue The value part of a name=value request option.
     * @param pvCopy The PVCopy to which the PVFilter will be attached.
     * @param master The field in the master PVStructure to which the PVFilter will be attached
     * @return The PVFilter.
     * Null is returned if master or requestValue is not appropriate for the plugin.
     */
    virtual PVFilterPtr create(
         const std::string & requestValue,
         const PVCopyPtr & pvCopy,
         const epics::pvData::PVFieldPtr & master);
};

/**
 * @brief  A filter that sets a timeStamp to/from the current field or pvCopy.
 */
class epicsShareClass PVTimestampFilter : public PVFilter
{
private:
    epics::pvData::PVTimeStamp pvTimeStamp;
    epics::pvData::TimeStamp timeStamp;
    bool current;
    bool copy;
    epics::pvData::PVFieldPtr master;


    PVTimestampFilter(bool current,bool copy,epics::pvData::PVFieldPtr const & pvField);
public:
    POINTER_DEFINITIONS(PVTimestampFilter);
    virtual ~PVTimestampFilter();
    /**
     * Create a PVTimestampFilter.
     * @param requestValue The value part of a name=value request option.
     * @param master The field in the master PVStructure to which the PVFilter will be attached.
     * @return The PVFilter.
     * A null is returned if master or requestValue is not appropriate for the plugin.
     */
    static PVTimestampFilterPtr create(const std::string & requestValue,const epics::pvData::PVFieldPtr & master);
    /**
     * Perform a filter operation
     * @param pvCopy The field in the copy PVStructure.
     * @param bitSet A bitSet for copyPVStructure.
     * @param toCopy (true,false) means copy (from master to copy,from copy to master)
     * @return if filter (modified, did not modify) destination.
     * Null is returned if master or requestValue is not appropriate for the plugin.
     */
    bool filter(const epics::pvData::PVFieldPtr & pvCopy,const epics::pvData::BitSetPtr & bitSet,bool toCopy);
    /**
     * Get the filter name.
     * @return The name.
     */
    std::string getName();
};

}}
#endif  /* PVTIMESTAMPPLUGIN_H */
