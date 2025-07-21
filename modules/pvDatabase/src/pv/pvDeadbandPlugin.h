/* pvDeadbandPlugin.h */
/*
 * The License for this software can be found in the file LICENSE that is included with the distribution.
 */

#ifndef PVDEADBANDPLUGIN_H
#define PVDEADBANDPLUGIN_H


#include <string>
#include <map>
#include <pv/lock.h>
#include <pv/pvData.h>
#include <pv/pvPlugin.h>

#include <shareLib.h>

namespace epics { namespace pvCopy{

class PVDeadbandPlugin;
class PVDeadbandFilter;

typedef std::tr1::shared_ptr<PVDeadbandPlugin> PVDeadbandPluginPtr;
typedef std::tr1::shared_ptr<PVDeadbandFilter> PVDeadbandFilterPtr;


/**
 * @brief  A plugin for a filter that gets a sub array from a PVScalarDeadband.
 *
 * @author mrk
 * @since date 2017.02.23
 */
class epicsShareClass PVDeadbandPlugin : public PVPlugin
{
private:
    PVDeadbandPlugin();
public:
    POINTER_DEFINITIONS(PVDeadbandPlugin);
    virtual ~PVDeadbandPlugin();
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
 * @brief  A Plugin for a filter that gets a sub array from a PVScalarDeadband.
 */
class epicsShareClass PVDeadbandFilter : public PVFilter
{
private:
    bool absolute;
    double deadband;
    epics::pvData::PVScalarPtr master;
    bool firstTime;
    double lastReportedValue;


    PVDeadbandFilter(bool absolute,double deadband,epics::pvData::PVScalarPtr const & master);
public:
    POINTER_DEFINITIONS(PVDeadbandFilter);
    virtual ~PVDeadbandFilter();
    /**
     * Create a PVDeadbandFilter.
     * @param requestValue The value part of a name=value request option.
     * @param master The field in the master PVStructure to which the PVFilter will be attached.
     * @return The PVFilter.
     * A null is returned if master or requestValue is not appropriate for the plugin.
     */
    static PVDeadbandFilterPtr create(
        const std::string & requestValue,
        const epics::pvData::PVFieldPtr & master);
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
#endif  /* PVDEADBANDPLUGIN_H */
