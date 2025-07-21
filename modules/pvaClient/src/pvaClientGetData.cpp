/* pvaClientGetData.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2015.02
 */

#include <typeinfo>
#include <sstream>

#include <pv/createRequest.h>
#include <pv/convert.h>

#define epicsExportSharedSymbols

#include <pv/pvaClient.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvaClient {


PvaClientGetDataPtr PvaClientGetData::create(StructureConstPtr const & structure)
{
    if(PvaClient::getDebug()) cout << "PvaClientGetData::create\n";
    PvaClientGetDataPtr epv(new PvaClientGetData(structure));
    return epv;
}

PvaClientGetData::PvaClientGetData(StructureConstPtr const & structure)
: PvaClientData(structure)
{}

}}
