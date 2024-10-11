/* pvPlugin.h */
/*
 * The License for this software can be found in the file LICENSE that is included with the distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2017.03
 */
#ifndef PVPLUGIN_H
#define PVPLUGIN_H

#include <string>
#include <map>
#include <pv/lock.h>
#include <pv/bitSet.h>

#include <shareLib.h>

namespace epics { namespace pvCopy{

class PVPlugin;
class PVFilter;
class PVPluginRegistry;

class PVCopy;
typedef std::tr1::shared_ptr<PVCopy> PVCopyPtr;

typedef std::tr1::shared_ptr<PVPlugin> PVPluginPtr;
typedef std::tr1::shared_ptr<PVFilter> PVFilterPtr;
typedef std::map<std::string,PVPluginPtr> PVPluginMap;


/**
 * @brief A filter plugin that attaches to a field of a PVStrcture.
 *
 * PVCopy looks for plugins defined in pvRequest and calls the filter when a pvCopy is updated.
 * @author mrk
 * @since 2017.03.17
 *
 * Interface for a filter plugin for PVCopy.
 *
 */

class epicsShareClass PVPlugin {
public:
    POINTER_DEFINITIONS(PVPlugin);
    virtual ~PVPlugin() {}
    /**
     * Create a PVFilter.
     * @param requestValue The value part of a name=value request option.
     * @param pvCopy The PVCopy to which the PVFilter will be attached.
     * @param master The field in the master PVStructure to which the PVFilter will be attached.
     * @return The PVFilter.
     * A null is returned if master or requestValue is not appropriate for the plugin.
     */
     virtual PVFilterPtr create(
         const std::string & requestValue,
         const PVCopyPtr & pvCopy,
         const epics::pvData::PVFieldPtr & master) = 0;
};

/**
 * @brief  A Filter that is called when a copy PVStructure is being updated.
 *
 * This interface defines a filter to update a copy of a field from a master PVStructure.
 * of the data in the master.
 */
class epicsShareClass PVFilter {
public:
    POINTER_DEFINITIONS(PVFilter);
    virtual ~PVFilter() {}
    /**
     * Update copy or master.
     * @param copy The data for copy.
     * @param bitSet The BitSet for copy.
     * @param toCopy (true,false) means copy (from master to copy,from copy to master)
     * @return (true,false) if filter modified destination.
     */
    virtual bool filter(const epics::pvData::PVFieldPtr & copy,const epics::pvData::BitSetPtr & bitSet,bool toCopy) = 0;
    /**
     * Get the filter name.
     * This is the name part of a request name=value pair.
     * @return The name.
     */
    virtual std::string getName() = 0;
};
/**
 * @brief  A registry for filter plugins for PVCopy.
 *
 */
class epicsShareClass PVPluginRegistry {
public:
    /**
     * Register a plugin.
     * @param name The name that appears in [name=value] of a field request option.
     * @param pvPlugin The implementation for the plugin.
     */
    static void registerPlugin(const std::string & name,const PVPluginPtr & pvPlugin);
    /**
     * Find a plugin.
     * @param name The name that appears in [name=value] of a field request option.
     * @return The plugin implementation or null if no pluging by that name has been registered.
     */
    static PVPluginPtr find(const std::string & name);
};

}}

#endif  /* PVPLUGIN_H */
