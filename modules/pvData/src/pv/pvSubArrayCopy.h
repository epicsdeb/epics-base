/* pvSubArrayCopy.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
/**
 * @author Marty Kraimer
 * @date 2013.07
 */
#ifndef PVSUBARRAYCOPY_H
#define PVSUBARRAYCOPY_H

#include <pv/pvData.h>

#include <shareLib.h>

namespace epics { namespace pvData { 

/** @brief Copy a subarray from one scalar array to another.
 * @warning The two scalar arrays must both be PVValueArrays of the same type.
 * @param pvFrom The source array.
 * @param fromOffset The offset in the source.
 * @param fromStride The interval between elements in pvFrom.
 * @param pvTo The destination array.
 * @param toOffset The offset in the destination.
 * @param toStride The interval between elements in pvTo.
 * @param count The total number of elements to copy from pvFrom to pvTo.
 */
template<typename T>
epicsShareExtern void copy(
    PVValueArray<T> & pvFrom,
    size_t fromOffset,
    size_t fromStride,
    PVValueArray<T> & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count);

/** @brief Copy a subarray from one scalar array to another.
 * @warning The two scalar arrays must both be PVValueArrays of the same type.
 * @param pvFrom The source array.
 * @param fromOffset The offset in the source.
 * @param fromStride The interval between elements in pvFrom.
 * @param pvTo The destination array.
 * @param toOffset The offset in the destination.
 * @param toStride The interval between elements in pvTo.
 * @param count The total number of elements to copy from pvFrom to pvTo.
 */
epicsShareExtern void copy(
    PVScalarArray & pvFrom,
    size_t fromOffset,
    size_t fromStride,
    PVScalarArray & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count);

/** @brief Copy a subarray from one structure array to another.
 * @warning The two structure arrays must have the same
 * structure introspection interface.
 * @param pvFrom The source array.
 * @param fromOffset The offset in the source.
 * @param fromStride The interval between elements in pvFrom.
 * @param pvTo The destination array.
 * @param toOffset The offset in the destination.
 * @param toStride The interval between elements in pvTo.
 * @param count The total number of elements to copy from pvFrom to pvTo.
 */
epicsShareExtern void copy(
    PVStructureArray & pvFrom,
    size_t fromOffset,
    size_t fromStride,
    PVStructureArray & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count);

/** @brief Copy a subarray from one  array to another.
 * @warning The two arrays must have the same
 * introspection interface.
 * @param pvFrom The source array.
 * @param fromOffset The offset in the source.
 * @param fromStride The interval between elements in pvFrom.
 * @param pvTo The destination array.
 * @param toOffset The offset in the destination.
 * @param toStride The interval between elements in pvTo.
 * @param count The total number of elements to copy from pvFrom to pvTo.
 */
epicsShareExtern void copy(
    PVArray & pvFrom,
    size_t fromOffset,
    size_t fromStride,
    PVArray & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count);

/** @brief Copy a subarray from one  array to another.
 * @warning The two arrays must have the same
 * introspection interface.
 * @param pvFrom The source array.
 * @param fromOffset The offset in the source.
 * @param fromStride The interval between elements in pvFrom.
 * @param pvTo The destination array.
 * @param toOffset The offset in the destination.
 * @param toStride The interval between elements in pvTo.
 * @param count The total number of elements to copy from pvFrom to pvTo.
 */
epicsShareExtern void copy(
    PVArray::shared_pointer const & pvFrom,
    size_t fromOffset,
    size_t fromStride,
    PVArray::shared_pointer & pvTo,
    size_t toOffset,
    size_t toStride,
    size_t count);

}}


#endif  /* PVSUBARRAYCOPY_H */
