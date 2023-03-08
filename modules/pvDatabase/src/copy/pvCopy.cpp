/* pvCopy.cpp */
/*
 * License terms for this software can be found in the file LICENSE that is included with the distribution
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>

#include <epicsThread.h>
#include <pv/pvData.h>
#include <pv/bitSet.h>
#include <pv/thread.h>

#define epicsExportSharedSymbols
#include "pv/pvPlugin.h"
#include "pv/pvStructureCopy.h"

using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::string;
using std::size_t;
using std::cout;
using std::endl;
using std::vector;
using namespace epics::pvData;

namespace epics { namespace pvCopy {

/**
 * Convenience method for implementing dump.
 * It generates a newline and inserts blanks at the beginning of the newline.
 * @param builder The std::string * being constructed.
 * @param indentLevel Indent level, Each level is four spaces.
 */
static void newLine(string *buffer, int indentLevel)
{
    *buffer += "\n";
    *buffer += string(indentLevel*4, ' ');
}

static PVCopyPtr NULLPVCopy;
static StructureConstPtr NULLStructure;
static PVStructurePtr NULLPVStructure;

struct CopyNode {
    CopyNode()
    : isStructure(false),
      structureOffset(0),
      nfields(0)
    {}
    PVFieldPtr masterPVField;
    bool isStructure;
    size_t structureOffset; // In the copy
    size_t nfields;
    PVStructurePtr options;
    vector<PVFilterPtr> pvFilters;
};

static CopyNodePtr NULLCopyNode;

typedef std::vector<CopyNodePtr> CopyNodePtrArray;
typedef std::tr1::shared_ptr<CopyNodePtrArray> CopyNodePtrArrayPtr;

struct CopyStructureNode : public  CopyNode {
    CopyNodePtrArrayPtr nodes;
};

PVCopyPtr PVCopy::create(
    PVStructurePtr const &pvMaster,
    PVStructurePtr const &pvRequest,
    string const & structureName)
{
    PVStructurePtr pvStructure(pvRequest);
    if(structureName.size()>0) {
        if(pvStructure->getStructure()->getNumberFields()>0) {
            pvStructure = pvRequest->getSubField<PVStructure>(structureName);
            if(!pvStructure) return NULLPVCopy;
        }
    } else if(pvRequest->getSubField<PVStructure>("field")) {
        pvStructure = pvRequest->getSubField<PVStructure>("field");
    }
    PVCopyPtr pvCopy = PVCopyPtr(new PVCopy(pvMaster));
    bool result = pvCopy->init(pvStructure);
    if(!result) return PVCopyPtr();
    pvCopy->traverseMasterInitPlugin();
    return pvCopy;
}

PVStructurePtr PVCopy::getPVMaster()
{
    return pvMaster;
}

void PVCopy::traverseMaster(PVCopyTraverseMasterCallbackPtr const & callback)
{
    traverseMaster(headNode,callback);
}

StructureConstPtr PVCopy::getStructure()
{
    return structure;
}

PVStructurePtr PVCopy::createPVStructure()
{
    if(cacheInitStructure) {
        PVStructurePtr save = cacheInitStructure;
        cacheInitStructure.reset();
        return save;
    }
    PVStructurePtr pvStructure =
        getPVDataCreate()->createPVStructure(structure);
    return pvStructure;
}


size_t PVCopy::getCopyOffset(PVFieldPtr const &masterPVField)
{
    if(!headNode->isStructure) {
        CopyNodePtr node = static_pointer_cast<CopyNode>(headNode);
        if((node->masterPVField.get())==masterPVField.get()) {
             return headNode->structureOffset;
        }
        PVStructure * parent = masterPVField->getParent();
        size_t offsetParent = parent->getFieldOffset();
        size_t off = masterPVField->getFieldOffset();
        size_t offdiff = off -offsetParent;
        if(offdiff<node->nfields) return headNode->structureOffset + offdiff;
        return string::npos;
    }
    CopyStructureNodePtr structNode = static_pointer_cast<CopyStructureNode>(headNode);
    CopyNodePtr node = getCopyOffset(structNode,masterPVField);
    if(node) return node->structureOffset;
    return string::npos;
}

size_t PVCopy::getCopyOffset(
    PVStructurePtr const  &masterPVStructure,
    PVFieldPtr const  &masterPVField)
{
    CopyNodePtr node;
    if(!headNode->isStructure) {
        node = static_pointer_cast<CopyNode>(headNode);
        if(node->masterPVField.get()!=masterPVStructure.get()) return string::npos;
    } else {
        CopyStructureNodePtr snode = static_pointer_cast<CopyStructureNode>(headNode);
        node = getCopyOffset(snode,masterPVField);
    }
    if(!node) return string::npos;
    size_t diff = masterPVField->getFieldOffset()
        - masterPVStructure->getFieldOffset();
    return node->structureOffset + diff;
}

PVFieldPtr PVCopy::getMasterPVField(size_t structureOffset)
{
    CopyNodePtr node;
    if(!headNode->isStructure) {
        node = headNode;
    } else {
        CopyStructureNodePtr snode = static_pointer_cast<CopyStructureNode>(headNode);
        node = getMasterNode(snode,structureOffset);
    }
    if(!node) {
        throw std::logic_error(
            "PVCopy::getMasterPVField: structureOffset not valid");
    }
    size_t diff = structureOffset - node->structureOffset;
    PVFieldPtr pvMasterField = node->masterPVField;
    if(diff==0) return pvMasterField;
    PVStructurePtr pvStructure
        = static_pointer_cast<PVStructure>(pvMasterField);
    return pvStructure->getSubField(
        pvMasterField->getFieldOffset() + diff);
}

void PVCopy::initCopy(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
    for(size_t i=0; i< copyPVStructure->getNumberFields(); ++i) {
        bitSet->set(i,true);
    }
    updateCopyFromBitSet(copyPVStructure,headNode,bitSet);
}


bool PVCopy::updateCopySetBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
    updateCopySetBitSet(copyPVStructure,headNode,bitSet);
    return checkIgnore(copyPVStructure,bitSet);
}

bool PVCopy::updateCopyFromBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
    if(bitSet->get(0)) {
        for(size_t i=0; i< copyPVStructure->getNumberFields(); ++i) {
            bitSet->set(i,true);
        }
    }
    updateCopyFromBitSet(copyPVStructure,headNode,bitSet);
    return checkIgnore(copyPVStructure,bitSet);
}

void PVCopy::updateMasterField(
     CopyNodePtr const & node,
     PVFieldPtr const & pvCopy,
     PVFieldPtr const &pvMaster,
     BitSetPtr const  &bitSet)
{
    bool result = false;
    for(size_t i=0; i< node->pvFilters.size(); ++i) {
        PVFilterPtr pvFilter = node->pvFilters[i];
        if(pvFilter->filter(pvCopy,bitSet,false)) result = true;
    }
    if(result) return;
    pvMaster->copyUnchecked(*pvCopy);
}

void PVCopy::updateMasterCheckBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet,
    size_t nextSet)
{
    if(!bitSet->get(nextSet)) {
        size_t next = bitSet->nextSetBit(nextSet);
        if(next==string::npos) return;
        updateMasterCheckBitSet(copyPVStructure,bitSet,next);
        return;
    }
    PVFieldPtr pvField = copyPVStructure;
    if(nextSet!=0) pvField = copyPVStructure->getSubField(nextSet);
    if(pvField->getField()->getType()==epics::pvData::structure) {
        bitSet->clear(nextSet);
        PVStructurePtr pv = static_pointer_cast<PVStructure>(pvField);
        PVFieldPtrArray pvFieldArray = pv->getPVFields();
        for(size_t i=0; i<pvFieldArray.size(); ++i) {
            PVFieldPtr pvField = pvFieldArray[i];
            bitSet->set(pvField->getFieldOffset());
        }
    }
    size_t next = bitSet->nextSetBit(nextSet+1);
    if(next==string::npos) return;
    updateMasterCheckBitSet(copyPVStructure,bitSet,next);
}

CopyNodePtr PVCopy::getCopyNode(std::size_t fieldOffset)
{
    if(fieldOffset==0) return headNode;
    CopyNodePtr node = headNode;
    while(true) {
        if(!node->isStructure) return node;
        CopyStructureNodePtr structNode = static_pointer_cast<CopyStructureNode>(node);
        CopyNodePtrArrayPtr nodes = structNode->nodes;
        bool okToContinue = false;
        for(size_t i=0; i< nodes->size(); i++) {
            node = (*nodes)[i];
            size_t soff = node->structureOffset;
            if(fieldOffset>=soff && fieldOffset<soff+node->nfields) {
                okToContinue = true;
                break;
            }
        }
        if(okToContinue) continue;
    }
    throw std::logic_error("PVCopy::getCopyNode fieldOffset not valid");
}


void PVCopy::updateMaster(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
    updateMasterCheckBitSet(copyPVStructure,bitSet,0);
    size_t nextSet =0;
    while(true) {
        nextSet = bitSet->nextSetBit(nextSet);
        if(nextSet==string::npos) return;
        PVFieldPtr pvCopy = copyPVStructure->getSubField(nextSet);
        PVFieldPtr pvMaster = headNode->masterPVField;
        if(pvMaster->getField()->getType()==epics::pvData::structure) {
             PVStructurePtr pv = static_pointer_cast<PVStructure>(pvMaster);
             pvMaster = pv->getSubField(pvCopy->getFullName());
        }
        updateMasterField(getCopyNode(nextSet),pvCopy,pvMaster,bitSet);
        bitSet->clear(nextSet);
    }
}

PVStructurePtr PVCopy::getOptions(std::size_t fieldOffset)
{
    if(fieldOffset==0) return headNode->options;
    CopyNodePtr node = headNode;
    while(true) {
        if(node->structureOffset==fieldOffset) return node->options;
        if(!node->isStructure) return NULLPVStructure;
        CopyStructureNodePtr structNode = static_pointer_cast<CopyStructureNode>(node);
        CopyNodePtrArrayPtr nodes = structNode->nodes;
        bool okToContinue = false;
        for(size_t i=0; i< nodes->size(); i++) {
            node = (*nodes)[i];
            size_t soff = node->structureOffset;
            if(fieldOffset>=soff && fieldOffset<soff+node->nfields) {
                if(fieldOffset==soff) return node->options;
                if(!node->isStructure) {
                    return NULLPVStructure;
                }
                okToContinue = true;
                break;
            }
        }
        if(okToContinue) continue;
        throw std::logic_error("PVCopy::getOptions logic error: fieldOffset not valid");
    }
}

string PVCopy::dump()
{
    string builder;
    dump(&builder,headNode,0);
    return builder;
}

void PVCopy::traverseMaster(
    CopyNodePtr const &innode,
    PVCopyTraverseMasterCallbackPtr const & callback)
{
    CopyNodePtr node = innode;
    if(!node->isStructure) {
        callback->nextMasterPVField(node->masterPVField);
        return;
    }
    CopyStructureNodePtr structNode = static_pointer_cast<CopyStructureNode>(node);
    CopyNodePtrArrayPtr nodes = structNode->nodes;
    for(size_t i=0; i< nodes->size(); i++) {
        node = (*nodes)[i];
        traverseMaster(node,callback);
    }
}

void PVCopy::updateCopySetBitSet(
    PVFieldPtr const & pvCopy,
    PVFieldPtr const & pvMaster,
    BitSetPtr const & bitSet)
{
    if(pvCopy->getField()->getType()!=epics::pvData::structure) {
        if(*pvCopy==*pvMaster) return;
        pvCopy->copy(*pvMaster);
        bitSet->set(pvCopy->getFieldOffset());
        return;
    }
    PVStructurePtr pvCopyStructure = static_pointer_cast<PVStructure>(pvCopy);
    PVFieldPtrArray const & pvCopyFields = pvCopyStructure->getPVFields();
    for(size_t i=0; i<pvCopyFields.size(); ++i) {
        PVFieldPtr master = getMasterPVField(pvCopyFields[i]->getFieldOffset());
        updateCopySetBitSet(pvCopyFields[i],master,bitSet);
    }
}

void PVCopy::updateCopySetBitSet(
    PVFieldPtr const & pvCopy,
    CopyNodePtr const & node,
    BitSetPtr const & bitSet)
{
    bool result = false;
    for(size_t i=0; i< node->pvFilters.size(); ++i) {
        PVFilterPtr pvFilter = node->pvFilters[i];
        if(pvFilter->filter(pvCopy,bitSet,true)) result = true;
    }
    if(!node->isStructure) {
        if(result) return;
        updateCopySetBitSet(pvCopy,node->masterPVField,bitSet);
        return;
    }
    CopyStructureNodePtr structureNode = static_pointer_cast<CopyStructureNode>(node);
    PVStructurePtr pvCopyStructure = static_pointer_cast<PVStructure>(pvCopy);
    PVFieldPtrArray const & pvCopyFields = pvCopyStructure->getPVFields();
    for(size_t i=0; i<pvCopyFields.size(); ++i) {
        updateCopySetBitSet(pvCopyFields[i],(*structureNode->nodes)[i],bitSet);
    }
}


void PVCopy::updateCopyFromBitSet(
    PVFieldPtr const & pvCopy,
    CopyNodePtr const & node,
    BitSetPtr const & bitSet)
{
    bool result = false;
    bool update = bitSet->get(pvCopy->getFieldOffset());
    if(update) {
        for(size_t i=0; i< node->pvFilters.size(); ++i) {
            PVFilterPtr pvFilter = node->pvFilters[i];
            if(pvFilter->filter(pvCopy,bitSet,true)) result = true;
        }
    }
    if(!node->isStructure) {
        if(result) return;
        PVFieldPtr pvMaster = node->masterPVField;
        pvCopy->copy(*pvMaster);
        return;
    }
    CopyStructureNodePtr structureNode = static_pointer_cast<CopyStructureNode>(node);
    size_t offset = structureNode->structureOffset;
    size_t nextSet = bitSet->nextSetBit(offset);
    if(nextSet==string::npos) return;
    if(offset>=pvCopy->getNextFieldOffset()) return;
    PVStructurePtr pvCopyStructure = static_pointer_cast<PVStructure>(pvCopy);
    PVFieldPtrArray const & pvCopyFields = pvCopyStructure->getPVFields();
    for(size_t i=0; i<pvCopyFields.size(); ++i) {
        updateCopyFromBitSet(pvCopyFields[i],(*structureNode->nodes)[i],bitSet);
    }
}

PVCopy::PVCopy(
    PVStructurePtr const &pvMaster)
: pvMaster(pvMaster)
{
}

bool PVCopy::init(epics::pvData::PVStructurePtr const &pvRequest)
{
    PVStructurePtr pvMasterStructure = pvMaster;
    size_t len = pvRequest->getPVFields().size();
    bool entireMaster = false;
    PVStructurePtr pvOptions;
    if(len==0) {
        entireMaster = true;
    }
    else {
        // If "_" is in the request, but not in the master structure,
        // then assume the top level PV structure is requested
        PVStructurePtr masterFieldPtr = pvMaster->getSubField<PVStructure>("_");
        PVStructurePtr requestFieldPtr = pvRequest->getSubField<PVStructure>("_");
        if (!masterFieldPtr && requestFieldPtr) {
            entireMaster = true;
            pvOptions = requestFieldPtr->getSubField<PVStructure>("_options");
        }
    }
    if(entireMaster) {
        structure = pvMasterStructure->getStructure();
        CopyNodePtr node(new CopyNode());
        headNode = node;
        node->options = pvOptions;
        node->isStructure = false;
        node->structureOffset = 0;
        node->masterPVField = pvMasterStructure;
        node->nfields = pvMasterStructure->getNumberFields();
        return true;
    }
    structure = createStructure(pvMasterStructure,pvRequest);
    if(!structure) return false;
    cacheInitStructure = createPVStructure();
    ignorechangeBitSet = BitSetPtr(new BitSet(cacheInitStructure->getNumberFields()));
    headNode = createStructureNodes(
        pvMaster,
        pvRequest,
        cacheInitStructure);
    return true;
}


StructureConstPtr PVCopy::createStructure(
    PVStructurePtr const &pvMaster,
    PVStructurePtr const  &pvFromRequest)
{
    if(pvFromRequest->getStructure()->getNumberFields()==0) {
        return pvMaster->getStructure();
    }
    PVFieldPtrArray const &pvFromRequestFields = pvFromRequest->getPVFields();
    StringArray const &fromRequestFieldNames = pvFromRequest->getStructure()->getFieldNames();
    size_t length = pvFromRequestFields.size();
    if(length==0) return NULLStructure;
    FieldConstPtrArray fields; fields.reserve(length);
    StringArray fieldNames; fieldNames.reserve(length);
    for(size_t i=0; i<length; ++i) {
        string const &fieldName = fromRequestFieldNames[i];
        PVFieldPtr pvMasterField = pvMaster->getSubField(fieldName);
        if(!pvMasterField) continue;
        FieldConstPtr field = pvMasterField->getField();
        if(field->getType()==epics::pvData::structure) {
            PVStructurePtr pvRequestStructure = static_pointer_cast<PVStructure>(
                pvFromRequestFields[i]);
            if(pvRequestStructure->getNumberFields()>0) {
                 StringArray const &names = pvRequestStructure->getStructure()->
                     getFieldNames();
                 size_t num = names.size();
                 if(num>0 && names[0].compare("_options")==0) --num;
                 if(num>0) {
                     if(pvMasterField->getField()->getType()!=epics::pvData::structure) continue;
                     fieldNames.push_back(fieldName);
                     fields.push_back(createStructure(
                         static_pointer_cast<PVStructure>(pvMasterField),
                         pvRequestStructure));
                     continue;
                 }
            }
        }
        fieldNames.push_back(fieldName);
        fields.push_back(field);
    }
    size_t numsubfields = fields.size();
    if(numsubfields==0) {
         std::stringstream ss;
         ss << pvFromRequest << "\n";
         string val("no fields from the following request were found\n");
         val += ss.str();
         throw std::invalid_argument(val);
    }
    return getFieldCreate()->createStructure(fieldNames, fields);
}

CopyNodePtr PVCopy::createStructureNodes(
    PVStructurePtr const &pvMasterStructure,
    PVStructurePtr const &pvFromRequest,
    PVStructurePtr const &pvFromCopy)
{
    PVFieldPtrArray const & copyPVFields = pvFromCopy->getPVFields();
    PVStructurePtr pvOptions = pvFromRequest->getSubField<PVStructure>("_options");
    size_t number = copyPVFields.size();
    CopyNodePtrArrayPtr nodes(new CopyNodePtrArray());
    nodes->reserve(number);
    for(size_t i=0; i<number; i++) {
        PVFieldPtr copyPVField = copyPVFields[i];
        string fieldName = copyPVField->getFieldName();
        PVStructurePtr requestPVStructure =
             pvFromRequest->getSubField<PVStructure>(fieldName);
        PVStructurePtr pvSubFieldOptions =
            requestPVStructure->getSubField<PVStructure>("_options");
        PVFieldPtr pvMasterField = pvMasterStructure->getSubField(fieldName);
        if(!pvMasterField) {
              throw std::logic_error("PVCopy::createStructureNodes did not find field in master");
        }
        size_t numberRequest = requestPVStructure->getPVFields().size();
        bool haveOptions = false;
        if(pvSubFieldOptions) {
             numberRequest--;
             haveOptions = true;
        }
        if(numberRequest>0) {
            Type copyType = copyPVField->getField()->getType();
            if(copyType==epics::pvData::structure) {
                nodes->push_back(createStructureNodes(
                    static_pointer_cast<PVStructure>(pvMasterField),
                    requestPVStructure,
                    static_pointer_cast<PVStructure>(copyPVField)));
                continue;
            }
            if(copyType==epics::pvData::union_) {
                 if(numberRequest!=1) {
                     std::stringstream ss;
                     ss << pvFromRequest << "\n";
                     string val("In the following request a union field has more than one subfield in\n");
                     val += ss.str();
                     throw std::invalid_argument(val);
                 }
                 PVUnionPtr pvUnion = static_pointer_cast<PVUnion>(pvMasterField);
                 std::string selectedName = pvUnion->getSelectedFieldName();
                 PVFieldPtrArray const & pvFields = requestPVStructure->getPVFields();
                 size_t len = pvFields.size();
                 if(len>2 || (haveOptions && len!=2)) {
                      std::stringstream ss;
                      ss << pvFromRequest << "\n";
                      string val("PVCopy logic error: pvRequest is\n");
                      val += ss.str();
                      throw std::logic_error(val);
                 }
                 size_t indRequestValue = 0;
                 if((pvFields[0]->getFieldName().compare("_options"))==0) indRequestValue = 1;
                 PVFieldPtr pvRequestValue = pvFields[indRequestValue];
                 if(pvRequestValue) {
                     string requestName = pvRequestValue->getFieldName();
                     if(requestName.compare(selectedName)!=0) {
                         std::stringstream ss;
                         ss << pvFromCopy << "\n";
                         string requestName = pvRequestValue->getFieldName();
                         string val("field ");
                         val += requestName + " does not match union type in\n";
                         val += ss.str();
                         throw std::invalid_argument(val);
                     }
                 }
            } else {
                 std::stringstream ss;
                 ss << pvFromCopy << "\n";
                 string val("requested a subfield of field ");
                 val += fieldName + " which does not have type structure in\n";
                 val += ss.str();
                 throw std::invalid_argument(val);
            }
        }
        CopyNodePtr node(new CopyNode());
        node->options = pvSubFieldOptions;
        node->isStructure = false;
        node->masterPVField = pvMasterField;
        node->nfields = copyPVField->getNumberFields();
        node->structureOffset = copyPVField->getFieldOffset();
        nodes->push_back(node);
    }
    CopyStructureNodePtr structureNode(new CopyStructureNode());
    structureNode->masterPVField = pvMasterStructure;
    structureNode->isStructure = true;
    structureNode->nodes = nodes;
    structureNode->structureOffset = pvFromCopy->getFieldOffset();
    structureNode->nfields = pvFromCopy->getNumberFields();
    structureNode->options = pvOptions;
    return structureNode;
}

void PVCopy::initPlugin(
    CopyNodePtr const & node,
    PVStructurePtr const & pvOptions,
    PVFieldPtr const & pvMasterField)
{
    PVFieldPtrArray const & pvFields = pvOptions->getPVFields();
    size_t num = pvFields.size();
    vector<PVFilterPtr> pvFilters(num);
    size_t numfilter = 0;
    for(size_t i=0; i<num; ++i) {
         PVStringPtr pvOption = static_pointer_cast<PVString>(pvFields[i]);
         string name = pvOption->getFieldName();
         string value = pvOption->get();
         PVPluginPtr pvPlugin = PVPluginRegistry::find(name);
         if(!pvPlugin) {
            if(name.compare("ignore")==0) setIgnore(node);
            continue;
        }
        pvFilters[numfilter] = pvPlugin->create(value,shared_from_this(),pvMasterField);
        if(pvFilters[numfilter]) ++numfilter;
    }
    if(numfilter==0) return;
    node->pvFilters.resize(numfilter);
    for(size_t i=0; i<numfilter; ++i) node->pvFilters[i] = pvFilters[i];
}

void PVCopy::traverseMasterInitPlugin()
{
    traverseMasterInitPlugin(headNode);
}

void PVCopy::traverseMasterInitPlugin(CopyNodePtr const & node)
{
    PVFieldPtr pvField = node->masterPVField;
    PVStructurePtr pvOptions = node->options;
    if(pvOptions) initPlugin(node,pvOptions,pvField);
    if(!node->isStructure) return;
    CopyStructureNodePtr structureNode = static_pointer_cast<CopyStructureNode>(node);
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i< nodes->size(); i++) {
       traverseMasterInitPlugin((*nodes)[i]);
    }
}

CopyNodePtr PVCopy::getCopyOffset(
        CopyStructureNodePtr const &structureNode,
        PVFieldPtr const &masterPVField)
{
    size_t offset = masterPVField->getFieldOffset();
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i< nodes->size(); i++) {
        CopyNodePtr node = (*nodes)[i];
        if(!node->isStructure) {
            size_t off = node->masterPVField->getFieldOffset();
            size_t nextOffset = node->masterPVField->getNextFieldOffset();
            if(offset>= off && offset<nextOffset) return node;
        } else {
            CopyStructureNodePtr subNode =
                static_pointer_cast<CopyStructureNode>(node);
            CopyNodePtr node =
                getCopyOffset(subNode,masterPVField);
            if(node) return node;
        }
    }
    return NULLCopyNode;
}



bool PVCopy::checkIgnore(
     PVStructurePtr const & copyPVStructure,
     BitSetPtr const & bitSet)
{
    if(!ignorechangeBitSet) {
        return (bitSet->nextSetBit(0)<0) ? false : true;
    }
    int32 numFields = copyPVStructure->getNumberFields();
    BitSet temp(numFields);
    temp = *bitSet;
    int32 ind = 0;
    while(true) {
        ind = ignorechangeBitSet->nextSetBit(ind);
        if(ind<0) break;
        temp.clear(ind);
        ind++;
        if(ind>=numFields) break;
    }
    return (temp.nextSetBit(0)<0) ? false : true;
}

void PVCopy::setIgnore(CopyNodePtr const &node) {
    ignorechangeBitSet->set(node->structureOffset);
    if(node->isStructure) {
        CopyStructureNodePtr structureNode = static_pointer_cast<CopyStructureNode>(node);
         CopyNodePtrArrayPtr nodes = structureNode->nodes;
         for(size_t i=0; i<nodes->size(); ++i) {
            CopyNodePtr node = (*nodes)[i];
            setIgnore(node);        }
    } else {
        size_t num = node->masterPVField->getNumberFields();
        if(num>1) {
            for(size_t i=1; i<num; ++i) {
                ignorechangeBitSet->set(node->structureOffset+i);
            }
        }
    }
}


CopyNodePtr PVCopy::getMasterNode(
        CopyStructureNodePtr const &structureNode,
        std::size_t structureOffset)
{
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i<nodes->size(); ++i) {
        CopyNodePtr node = (*nodes)[i];
        if(structureOffset>=(node->structureOffset + node->nfields)) continue;
        if(!node->isStructure) return node;
        CopyStructureNodePtr subNode =
            static_pointer_cast<CopyStructureNode>(node);
        return  getMasterNode(subNode,structureOffset);
    }
    return NULLCopyNode;
}

void PVCopy::dump(string *builder,CopyNodePtr const &node,int indentLevel)
{
    newLine(builder,indentLevel);
    std::stringstream ss;
    ss << (node->isStructure ? "structureNode" : "node");
    ss << " structureOffset " << node->structureOffset;
    ss << " nfields " << node->nfields;
    *builder +=  ss.str();
    PVStructurePtr options = node->options;
    if(options) {
        newLine(builder,indentLevel +1);
        *builder += options->getFieldName();
        PVFieldPtrArray pvFields = options->getPVFields();
        for(size_t i=0; i< pvFields.size() ; ++i) {
           PVStringPtr pvString = static_pointer_cast<PVString>(pvFields[i]);
           newLine(builder,indentLevel +2);
           *builder += pvString->getFieldName() + " " + pvString->get();
        }
    }
    string name = node->masterPVField->getFullName();
    newLine(builder,indentLevel +1);
    *builder += "masterField " + name;
    if(node->pvFilters.size()>0) {
        newLine(builder,indentLevel +2);
        *builder += "filters:";
        for(size_t i=0; i< node->pvFilters.size(); ++i) {
            PVFilterPtr pvFilter = node->pvFilters[i];
            *builder += " " + pvFilter->getName();
        }
    }
    if(!node->isStructure) return;
    CopyStructureNodePtr structureNode =
        static_pointer_cast<CopyStructureNode>(node);
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i<nodes->size(); ++i) {
        CopyNodePtr node = (*nodes)[i];
        if(!node) {
            newLine(builder,indentLevel +1);
            ss.str("");
            ss << "node[" << i << "] is null";
            *builder += ss.str();
            continue;
        }
        dump(builder,node,indentLevel+1);
    }
}


}}
