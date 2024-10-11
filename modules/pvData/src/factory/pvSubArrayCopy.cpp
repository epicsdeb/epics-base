/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Marty Kraimer
 * @date 2013.07
 */
#include <string>
#include <stdexcept>
#include <memory>

#define epicsExportSharedSymbols
#include <pv/pvSubArrayCopy.h>

using std::cout;
using std::endl;
using std::string;

namespace epics { namespace pvData {

template<typename T>
void copy(
    PVValueArray<T> & pvFrom,
    size_t fromOffset,
    size_t fromStride,
    PVValueArray<T> & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count)
{
    if(pvTo.isImmutable()) throw std::invalid_argument("pvSubArrayCopy: pvTo is immutable");
    if(fromStride<1 || toStride<1) throw std::invalid_argument("stride must be >=1");
    size_t fromLength = pvFrom.getLength();
    size_t maxcount = (fromLength -fromOffset + fromStride -1)/fromStride;
    if(count>maxcount) throw std::invalid_argument("pvSubArrayCopy pvFrom length error");
    size_t newLength = toOffset + count*toStride;
    size_t capacity = pvTo.getCapacity();
    if(newLength>capacity) capacity = newLength;
    shared_vector<T> temp(capacity);
    typename PVValueArray<T>::const_svector vecFrom = pvFrom.view();
    typename PVValueArray<T>::const_svector vecTo = pvTo.view();
    for(size_t i=0; i<pvTo.getLength(); ++i) temp[i] = vecTo[i];
    for(size_t i=pvTo.getLength(); i< capacity; ++i) temp[i] = T();
    for(size_t i=0; i<count; ++i) temp[i*toStride + toOffset] = vecFrom[i*fromStride+fromOffset];
    shared_vector<const T> temp2(freeze(temp));
    pvTo.replace(temp2);
}

void copy(
    PVScalarArray & from,
    size_t fromOffset,
    size_t fromStride,
    PVScalarArray & to,
    size_t toOffset,
    size_t toStride,
    size_t count)
{
    ScalarType scalarType = from.getScalarArray()->getElementType();
    ScalarType otherType = to.getScalarArray()->getElementType();
    if(scalarType!=otherType) {
        throw std::invalid_argument("pvSubArrayCopy element types do not match");
    }
    switch(scalarType)
    {
    case pvBoolean:
       {
           copy(dynamic_cast<PVValueArray<boolean> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<boolean>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvByte:
       {
           copy(dynamic_cast<PVValueArray<int8> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<int8>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvShort:
       {
           copy(dynamic_cast<PVValueArray<int16> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<int16>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvInt:
       {
           copy(dynamic_cast<PVValueArray<int32> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<int32>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvLong:
       {
           copy(dynamic_cast<PVValueArray<int64> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<int64>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvUByte:
       {
           copy(dynamic_cast<PVValueArray<uint8> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<uint8>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvUShort:
       {
           copy(dynamic_cast<PVValueArray<uint16> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<uint16>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvUInt:
       {
           copy(dynamic_cast<PVValueArray<uint32> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<uint32>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvULong:
       {
           copy(dynamic_cast<PVValueArray<uint64> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<uint64>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvFloat:
       {
           copy(dynamic_cast<PVValueArray<float> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<float>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvDouble:
       {
           copy(dynamic_cast<PVValueArray<double> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<double>& >(to),
           toOffset,toStride,count);
       }
       break;
    case pvString:
       {
           copy(dynamic_cast<PVValueArray<string> &>(from),fromOffset,fromStride,
           dynamic_cast<PVValueArray<string>& >(to),
           toOffset,toStride,count);
       }
       break;
    }
}

void copy(
    PVStructureArray & pvFrom,
    size_t pvFromOffset,
    size_t pvFromStride,
    PVStructureArray & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count)
{
    if(pvTo.isImmutable()) {
        throw std::logic_error("pvSubArrayCopy  pvTo is immutable");
    }
    if(pvFromStride<1 || toStride<1) throw std::invalid_argument("stride must be >=1");
    StructureArrayConstPtr pvFromStructure = pvFrom.getStructureArray();
    StructureArrayConstPtr toStructure = pvTo.getStructureArray();
    if(pvFromStructure->getStructure()!=toStructure->getStructure()) {
        throw std::invalid_argument(
            "pvSubArrayCopy structureArray pvTo and pvFrom have different structures");
    }
    size_t pvFromLength = pvFrom.getLength();
    size_t maxcount = (pvFromLength -pvFromOffset + pvFromStride -1)/pvFromStride;
    if(count>maxcount) throw std::invalid_argument("pvSubArrayCopy pvFrom length error");
    size_t newLength = toOffset + count*toStride;
    size_t capacity = pvTo.getCapacity();
    if(newLength>capacity) capacity = newLength;
    shared_vector<PVStructurePtr> temp(capacity);
    PVValueArray<PVStructurePtr>::const_svector vecFrom = pvFrom.view();
    PVValueArray<PVStructurePtr>::const_svector vecTo = pvTo.view();
    for(size_t i=0; i<pvTo.getLength(); ++i) temp[i] = vecTo[i];
    for(size_t i=pvTo.getLength(); i< capacity; ++i)
        temp[i] = getPVDataCreate()->createPVStructure(toStructure->getStructure());
    for(size_t i=0; i<count; ++i) temp[i*toStride + toOffset] = vecFrom[i*pvFromStride+pvFromOffset];
    shared_vector<const PVStructurePtr> temp2(freeze(temp));
    pvTo.replace(temp2);
}

void copy(
    PVUnionArray & pvFrom,
    size_t pvFromOffset,
    size_t pvFromStride,
    PVUnionArray & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count)
{
    if(pvTo.isImmutable()) {
        throw std::logic_error("pvSubArrayCopy  pvTo is immutable");
    }
    if(pvFromStride<1 || toStride<1) throw std::invalid_argument("stride must be >=1");
    UnionArrayConstPtr pvFromUnion = pvFrom.getUnionArray();
    UnionArrayConstPtr toUnion = pvTo.getUnionArray();
    if(pvFromUnion->getUnion()!=toUnion->getUnion()) {
        throw std::invalid_argument(
            "pvSubArrayCopy unionArray pvTo and pvFrom have different unions");
    }
    size_t pvFromLength = pvFrom.getLength();
    size_t maxcount = (pvFromLength -pvFromOffset + pvFromStride -1)/pvFromStride;
    if(count>maxcount) throw std::invalid_argument("pvSubArrayCopy pvFrom length error");
    size_t newLength = toOffset + count*toStride;
    size_t capacity = pvTo.getCapacity();
    if(newLength>capacity) capacity = newLength;
    shared_vector<PVUnionPtr> temp(capacity);
    PVValueArray<PVUnionPtr>::const_svector vecFrom = pvFrom.view();
    PVValueArray<PVUnionPtr>::const_svector vecTo = pvTo.view();
    for(size_t i=0; i<pvTo.getLength(); ++i) temp[i] = vecTo[i];
    for(size_t i=pvTo.getLength(); i< capacity; ++i)
        temp[i] = getPVDataCreate()->createPVUnion(toUnion->getUnion());
    for(size_t i=0; i<count; ++i) temp[i*toStride + toOffset] = vecFrom[i*pvFromStride+pvFromOffset];
    shared_vector<const PVUnionPtr> temp2(freeze(temp));
    pvTo.replace(temp2);
}

void copy(
    PVArray & pvFrom,
    size_t pvFromOffset,
    size_t pvFromStride,
    PVArray & pvTo,
    size_t pvToOffset,
    size_t pvToStride,
    size_t count)
{
    Type pvFromType = pvFrom.getField()->getType();
    Type pvToType = pvTo.getField()->getType();
    if(pvFromType!=pvToType) throw std::invalid_argument("pvSubArrayCopy: pvFrom and pvTo different types");
    if(pvFromType==scalarArray) {
        ScalarType pvFromScalarType= static_cast<ScalarType>(pvFromType);
        ScalarType pvToScalarType = static_cast<ScalarType>(pvToType);
        if(pvFromScalarType!=pvToScalarType){
              throw std::invalid_argument("pvSubArrayCopy: pvFrom and pvTo different types");
        }
    }
    if(pvTo.isImmutable()) throw std::invalid_argument("pvSubArrayCopy: pvTo is immutable");
    if(pvFromType==scalarArray) {
           copy(dynamic_cast<PVScalarArray &>(pvFrom) ,pvFromOffset,pvFromStride,
           dynamic_cast<PVScalarArray&>(pvTo),
           pvToOffset,pvToStride,count);
    }
    if(pvFromType==structureArray) {
           copy(dynamic_cast<PVStructureArray &>(pvFrom) ,pvFromOffset,pvFromStride,
           dynamic_cast<PVStructureArray&>(pvTo),
           pvToOffset,pvToStride,count);
    }
    if(pvFromType==unionArray) {
           copy(dynamic_cast<PVUnionArray &>(pvFrom) ,pvFromOffset,pvFromStride,
           dynamic_cast<PVUnionArray&>(pvTo),
           pvToOffset,pvToStride,count);
    }
}

void copy(
    PVArray::shared_pointer const & pvFrom,
    size_t pvFromOffset,
    size_t pvFromStride,
    PVArray::shared_pointer & pvTo,
    size_t pvToOffset,
    size_t pvToStride,
    size_t count)
{
    copy(*pvFrom,pvFromOffset,pvFromStride,*pvTo,pvToOffset,pvToStride,count);
}

}}
