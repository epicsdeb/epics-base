// Copyright information and license terms for this software can be
// found in the file LICENSE that is included with the distribution

#include <stdlib.h>

#include <cctype>
#include <string>
#include <algorithm>
#include <pv/lock.h>
#include <pv/pvData.h>
#include <pv/bitSet.h>

#define epicsExportSharedSymbols
#include "pv/dataDistributorPlugin.h"

using std::string;
using std::size_t;
using std::endl;
using std::tr1::static_pointer_cast;
using std::vector;
using namespace epics::pvData;
namespace epvd = epics::pvData;

namespace epics { namespace pvCopy {

// Utilities for manipulating strings
static std::string leftTrim(const std::string& s)
{
    unsigned int i;
    unsigned int n = (unsigned int)s.length();
    for (i = 0; i < n; i++) {
        if (!isspace(s[i])) {
            break;
        }
    }
    return s.substr(i,n-i);
}

static std::string rightTrim(const std::string& s)
{
    unsigned int i;
    unsigned int n = (unsigned int)s.length();
    for (i = n; i > 0; i--) {
        if (!isspace(s[i-1])) {
            break;
        }
    }
    return s.substr(0,i);
}

static std::string trim(const std::string& s)
{
    return rightTrim(leftTrim(s));
}

static std::vector<std::string>& split(const std::string& s, char delimiter, std::vector<std::string>& elements)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        elements.push_back(trim(item));
    }
    return elements;
}

static std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> elements;
    split(s, delimiter, elements);
    return elements;
}

static std::string toLowerCase(const std::string& input)
{
    std::stringstream ss;
    for (unsigned int i = 0; i < input.size(); i++) {
        char c = std::tolower(input.at(i));
        ss << c;
    }
    return ss.str();
}

// Data distributor class

static std::string name("distributor");
bool DataDistributorPlugin::initialized(false);

std::map<std::string, DataDistributorPtr> DataDistributor::dataDistributorMap;
epics::pvData::Mutex DataDistributor::dataDistributorMapMutex;

DataDistributorPtr DataDistributor::getInstance(const std::string& groupId)
{
    epvd::Lock lock(dataDistributorMapMutex);
    std::map<std::string,DataDistributorPtr>::iterator ddit = dataDistributorMap.find(groupId);
    if (ddit != dataDistributorMap.end()) {
        DataDistributorPtr ddPtr = ddit->second;
        return ddPtr;
    }
    else {
        DataDistributorPtr ddPtr(new DataDistributor(groupId));
        dataDistributorMap[groupId] = ddPtr;
        return ddPtr;
    }
}

void DataDistributor::removeUnusedInstance(DataDistributorPtr dataDistributorPtr)
{
    epvd::Lock lock(dataDistributorMapMutex);
    std::string groupId = dataDistributorPtr->getGroupId();
    std::map<std::string,DataDistributorPtr>::iterator ddit = dataDistributorMap.find(groupId);
    if (ddit != dataDistributorMap.end()) {
        DataDistributorPtr ddPtr = ddit->second;
        size_t nSets = ddPtr->clientSetMap.size();
        if (nSets == 0) {
            dataDistributorMap.erase(ddit);
        }
    }
}

DataDistributor::DataDistributor(const std::string& groupId_)
    : groupId(groupId_)
    , mutex()
    , clientSetMap()
    , clientSetIdList()
    , currentSetIdIter(clientSetIdList.end())
    , lastUpdateValue()
{
}

DataDistributor::~DataDistributor()
{
    epvd::Lock lock(mutex);
    clientSetMap.clear();
    clientSetIdList.clear();
}

std::string DataDistributor::addClient(int clientId, const std::string& setId, const std::string& triggerField, int nUpdatesPerClient, int updateMode)
{
    epvd::Lock lock(mutex);
    std::map<std::string,ClientSetPtr>::iterator git = clientSetMap.find(setId);
    if (git != clientSetMap.end()) {
        ClientSetPtr setPtr = git->second;
        setPtr->clientIdList.push_back(clientId);
        return setPtr->triggerField;
    }
    else {
        ClientSetPtr setPtr(new ClientSet(setId, triggerField, nUpdatesPerClient, updateMode));
        setPtr->clientIdList.push_back(clientId);
        clientSetMap[setId] = setPtr;
        clientSetIdList.push_back(setId);
        return triggerField;
    }
}

void DataDistributor::removeClient(int clientId, const std::string& setId)
{
    epvd::Lock lock(mutex);
    std::map<std::string,ClientSetPtr>::iterator git = clientSetMap.find(setId);
    if (git != clientSetMap.end()) {
        ClientSetPtr setPtr = git->second;
        std::list<int>::iterator cit = std::find(setPtr->clientIdList.begin(), setPtr->clientIdList.end(), clientId);
        if (cit != setPtr->clientIdList.end()) {
            // If we are removing current client id, advance iterator
            if (cit == setPtr->currentClientIdIter) {
                setPtr->currentClientIdIter++;
            }

            // Find current client id 
            int currentClientId = -1;
            if (setPtr->currentClientIdIter != setPtr->clientIdList.end()) {
                currentClientId = *(setPtr->currentClientIdIter);
            }

            // Remove client id from the list
            setPtr->clientIdList.erase(cit);

             // Reset current client id iterator
            setPtr->currentClientIdIter = setPtr->clientIdList.end();
            if (currentClientId >= 0) {
                std::list<int>::iterator cit2 = std::find(setPtr->clientIdList.begin(), setPtr->clientIdList.end(), currentClientId);
                if (cit2 != setPtr->clientIdList.end()) {
                    setPtr->currentClientIdIter = cit2;
                }
            }
        }
       
        if (setPtr->clientIdList.size() == 0) {
            clientSetMap.erase(git);
            std::list<std::string>::iterator git2 = std::find(clientSetIdList.begin(), clientSetIdList.end(), setId);
            if (git2 == currentSetIdIter) {
                currentSetIdIter++;
            }
            if (git2 != clientSetIdList.end()) {
                clientSetIdList.erase(git2);
            }
        }
    }
}

bool DataDistributor::updateClient(int clientId, const std::string& setId, const std::string& triggerFieldValue)
{
    epvd::Lock lock(mutex);
    bool proceedWithUpdate = false;
    if (currentSetIdIter == clientSetIdList.end()) {
        currentSetIdIter = clientSetIdList.begin();
    }
    std::string currentSetId = *currentSetIdIter;
    if (setId != currentSetId) {
        // We are not distributing data to this set at the moment
        return proceedWithUpdate;
    }
    ClientSetPtr setPtr = clientSetMap[currentSetId];
    if (setPtr->currentClientIdIter == setPtr->clientIdList.end()) {
        // Move current client iterator to the beginning of the list
        setPtr->currentClientIdIter = setPtr->clientIdList.begin();
    }
    if (lastUpdateValue == triggerFieldValue) {
        // This update was already distributed.
        return proceedWithUpdate;
    }
    switch (setPtr->updateMode) {
        case(DD_UPDATE_ONE_PER_GROUP): {
            if (clientId != *(setPtr->currentClientIdIter)) {
                // Not this client's turn.
                return proceedWithUpdate;
            }
            proceedWithUpdate = true;
            lastUpdateValue = triggerFieldValue;
            setPtr->lastUpdateValue = triggerFieldValue;
            setPtr->updateCounter++;
            if (setPtr->updateCounter >= setPtr->nUpdatesPerClient) {
                // This client and set are done.
                setPtr->currentClientIdIter++;
                setPtr->updateCounter = 0;
                currentSetIdIter++;
            }
            break;
        }
        case(DD_UPDATE_ALL_IN_GROUP): {
            proceedWithUpdate = true;
            static unsigned int nClientsUpdated = 0;
            if (setPtr->lastUpdateValue != triggerFieldValue) {
                setPtr->lastUpdateValue = triggerFieldValue;
                setPtr->updateCounter++;
                nClientsUpdated = 0;
            }
            nClientsUpdated++;
            if (nClientsUpdated == setPtr->clientIdList.size() && setPtr->updateCounter >= setPtr->nUpdatesPerClient) {
                // This set is done.
                lastUpdateValue = triggerFieldValue;
                setPtr->updateCounter = 0;
                currentSetIdIter++;
            }
            break;
        }
        default: {
            proceedWithUpdate = true;
        }
    }
    return proceedWithUpdate;
}

DataDistributorPlugin::DataDistributorPlugin()
{
}

DataDistributorPlugin::~DataDistributorPlugin()
{
}

void DataDistributorPlugin::create()
{
    initialize();
}

bool DataDistributorPlugin::initialize()
{
    if (!initialized) {
        initialized = true;
        DataDistributorPluginPtr pvPlugin = DataDistributorPluginPtr(new DataDistributorPlugin());
        PVPluginRegistry::registerPlugin(name,pvPlugin);
    }
    return true;
}

PVFilterPtr DataDistributorPlugin::create(
     const std::string& requestValue,
     const PVCopyPtr& pvCopy,
     const PVFieldPtr& master)
{
    return DataDistributorFilter::create(requestValue,pvCopy,master);
}

DataDistributorFilter::~DataDistributorFilter()
{
    dataDistributorPtr->removeClient(clientId, setId);
    DataDistributor::removeUnusedInstance(dataDistributorPtr);
}

DataDistributorFilterPtr DataDistributorFilter::create(
     const std::string& requestValue,
     const PVCopyPtr& pvCopy,
     const PVFieldPtr& master)
{
    static int clientId = 0;
    clientId++;
      
    std::vector<std::string> configItems = split(requestValue, ';');
    // Use lowercase keys if possible.
    std::string requestValue2 = toLowerCase(requestValue); 
    std::vector<std::string> configItems2 = split(requestValue2, ';');
    int nUpdatesPerClient = 1;
    int updateMode = DataDistributor::DD_UPDATE_ONE_PER_GROUP;
    std::string groupId = "default";
    std::string setId = "default";
    std::string triggerField = "timeStamp";
    bool hasUpdateMode = false;
    bool hasSetId = false;
    for(unsigned int i = 0; i < configItems2.size(); i++) {
        std::string configItem2 = configItems2[i];
        size_t ind = configItem2.find(':');
        if (ind == string::npos) {
            continue;
        }
        if(configItem2.find("updates") == 0) {
            std::string svalue = configItem2.substr(ind+1);
            nUpdatesPerClient = atoi(svalue.c_str());
        }
        else if(configItem2.find("group") == 0) {
            std::string configItem = configItems[i];
            groupId = configItem.substr(ind+1);
        }
        else if(configItem2.find("set") == 0) {
            std::string configItem = configItems[i];
            setId = configItem.substr(ind+1);
            hasSetId = true;
        }
        else if(configItem2.find("mode") == 0) {
            std::string svalue = toLowerCase(configItem2.substr(ind+1));
            if (svalue == "one") {
                updateMode = DataDistributor::DD_UPDATE_ONE_PER_GROUP;
                hasUpdateMode = true;
            }
            else if (svalue == "all") {
                updateMode = DataDistributor::DD_UPDATE_ALL_IN_GROUP;
                hasUpdateMode = true;
            }
        }
        else if(configItem2.find("trigger") == 0) {
            std::string configItem = configItems[i];
            triggerField = configItem.substr(ind+1);
        }
    }
    // If request does not have update mode specified, but has set id
    // then use a different update mode
    if(!hasUpdateMode && hasSetId) {
        updateMode = DataDistributor::DD_UPDATE_ALL_IN_GROUP;
    }

    // Make sure request is valid
    if(nUpdatesPerClient <= 0) {
        return DataDistributorFilterPtr();
    }
    DataDistributorFilterPtr filter =
         DataDistributorFilterPtr(new DataDistributorFilter(groupId, clientId, setId, triggerField, nUpdatesPerClient, updateMode, pvCopy, master));
    return filter;
}

DataDistributorFilter::DataDistributorFilter(const std::string& groupId_, int clientId_, const std::string& setId_, const std::string& triggerField_, int nUpdatesPerClient, int updateMode, const PVCopyPtr& copyPtr_, const epvd::PVFieldPtr& masterFieldPtr_)
    : dataDistributorPtr(DataDistributor::getInstance(groupId_))
    , clientId(clientId_)
    , setId(setId_)
    , triggerField(triggerField_)
    , masterFieldPtr(masterFieldPtr_)
    , triggerFieldPtr()
    , firstUpdate(true)
{
    triggerField = dataDistributorPtr->addClient(clientId, setId, triggerField, nUpdatesPerClient, updateMode);
    if(masterFieldPtr->getField()->getType() == epvd::structure) {
        epvd::PVStructurePtr pvStructurePtr = static_pointer_cast<epvd::PVStructure>(masterFieldPtr);
        if(pvStructurePtr) {
            triggerFieldPtr = pvStructurePtr->getSubField(triggerField);
        }
    }
    if(!triggerFieldPtr) {
        triggerFieldPtr = masterFieldPtr;
    }
}


bool DataDistributorFilter::filter(const PVFieldPtr& pvCopy, const BitSetPtr& bitSet, bool toCopy)
{
    if(!toCopy) {
        return false;
    }

    bool proceedWithUpdate = false;
    if(firstUpdate) {
        // Always send first update
        firstUpdate = false;
        proceedWithUpdate = true;
    }
    else {
        std::stringstream ss;
        ss << triggerFieldPtr;
        std::string triggerFieldValue = ss.str();
        proceedWithUpdate = dataDistributorPtr->updateClient(clientId, setId, triggerFieldValue);
    }

    if(proceedWithUpdate) {
        pvCopy->copyUnchecked(*masterFieldPtr);
        bitSet->set((unsigned int)pvCopy->getFieldOffset());
    } 
    else {
        // Clear all bits
        //bitSet->clear(pvCopy->getFieldOffset());
        bitSet->clear();
    }

    return true;
}

string DataDistributorFilter::getName()
{
    return name;
}

}}
