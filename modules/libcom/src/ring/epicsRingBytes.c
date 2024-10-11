/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2012 ITER Organization.
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 * Author:  Marty Kraimer Date:    15JUL99
 *          Eric Norum
 *          Ralph Lange <Ralph.Lange@gmx.de>
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "epicsSpin.h"
#include "dbDefs.h"
#include "epicsRingBytes.h"

/*
 * Need at least one extra byte to be able to distinguish a completely
 * full buffer from a completely empty one.  Allow for a little extra
 * space to try and keep good alignment and avoid multiple calls to
 * memcpy for a single put/get operation.
 */
#define SLOP    16

typedef struct ringPvt {
    epicsSpinId    lock;
    volatile int   nextPut;
    volatile int   nextGet;
    int            size;
    int            highWaterMark;
    volatile char buffer[1]; /* actually larger */
}ringPvt;

LIBCOM_API epicsRingBytesId  epicsStdCall epicsRingBytesCreate(int size)
{
    ringPvt *pring = malloc(sizeof(ringPvt) + size + SLOP);
    if(!pring)
        return NULL;
    pring->size = size + SLOP;
    pring->highWaterMark = 0;
    pring->nextGet = 0;
    pring->nextPut = 0;
    pring->lock    = 0;
    return((void *)pring);
}

LIBCOM_API epicsRingBytesId  epicsStdCall epicsRingBytesLockedCreate(int size)
{
    ringPvt *pring = (ringPvt *)epicsRingBytesCreate(size);
    if(!pring)
        return NULL;
    pring->lock = epicsSpinCreate();
    return((void *)pring);
}

LIBCOM_API void epicsStdCall epicsRingBytesDelete(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;
    if (pring->lock) epicsSpinDestroy(pring->lock);
    free((void *)pring);
}

LIBCOM_API int epicsStdCall epicsRingBytesGet(
    epicsRingBytesId id, char *value,int nbytes)
{
    ringPvt *pring = (ringPvt *)id;
    int nextGet, nextPut, size;
    int count;

    if (pring->lock) epicsSpinLock(pring->lock);
    nextGet = pring->nextGet;
    nextPut = pring->nextPut;
    size = pring->size;

    if (nextGet <= nextPut) {
        count = nextPut - nextGet;
        if (count < nbytes)
            nbytes = count;
        if (nbytes)
            memcpy (value, (void *)&pring->buffer[nextGet], nbytes);
        nextGet += nbytes;
    }
    else {
        count = size - nextGet;
        if (count > nbytes)
            count = nbytes;
        memcpy (value, (void *)&pring->buffer[nextGet], count);
        nextGet += count;
        if (nextGet == size) {
            int nLeft = nbytes - count;
            if (nLeft > nextPut)
                nLeft = nextPut;
            memcpy (value+count, (void *)&pring->buffer[0], nLeft);
            nextGet = nLeft;
            nbytes = count + nLeft;
        }
        else {
            nbytes = count;
        }
    }
    pring->nextGet = nextGet;

    if (pring->lock) epicsSpinUnlock(pring->lock);
    return nbytes;
}

LIBCOM_API int epicsStdCall epicsRingBytesPut(
    epicsRingBytesId id, char *value,int nbytes)
{
    ringPvt *pring = (ringPvt *)id;
    int nextGet, nextPut, size;
    int freeCount, copyCount, topCount, used;

    if (pring->lock) epicsSpinLock(pring->lock);
    nextGet = pring->nextGet;
    nextPut = pring->nextPut;
    size = pring->size;

    if (nextPut < nextGet) {
        freeCount = nextGet - nextPut - SLOP;
        if (nbytes > freeCount) {
            if (pring->lock) epicsSpinUnlock(pring->lock);
            return 0;
        }
        if (nbytes) {
            memcpy ((void *)&pring->buffer[nextPut], value, nbytes);
        }
        nextPut += nbytes;
    }
    else {
        freeCount = size - nextPut + nextGet - SLOP;
        if (nbytes > freeCount) {
            if (pring->lock) epicsSpinUnlock(pring->lock);
            return 0;
        }
        topCount = size - nextPut;
        copyCount = (nbytes > topCount) ?  topCount : nbytes;
        if (copyCount) {
            memcpy ((void *)&pring->buffer[nextPut], value, copyCount);
        }
        nextPut += copyCount;
        if (nextPut == size) {
            int nLeft = nbytes - copyCount;
            if (nLeft)
                memcpy ((void *)&pring->buffer[0], value+copyCount, nLeft);
            nextPut = nLeft;
        }
    }
    pring->nextPut = nextPut;

    used = nextPut - nextGet;
    if (used < 0) used += pring->size;
    if (used > pring->highWaterMark) pring->highWaterMark = used;

    if (pring->lock) epicsSpinUnlock(pring->lock);
    return nbytes;
}

LIBCOM_API void epicsStdCall epicsRingBytesFlush(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;

    if (pring->lock) epicsSpinLock(pring->lock);
    pring->nextGet = pring->nextPut;
    if (pring->lock) epicsSpinUnlock(pring->lock);
}

LIBCOM_API int epicsStdCall epicsRingBytesFreeBytes(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;
    int nextGet, nextPut;

    if (pring->lock) epicsSpinLock(pring->lock);
    nextGet = pring->nextGet;
    nextPut = pring->nextPut;
    if (pring->lock) epicsSpinUnlock(pring->lock);

    if (nextPut < nextGet)
        return nextGet - nextPut - SLOP;
    else
        return pring->size - nextPut + nextGet - SLOP;
}

LIBCOM_API int epicsStdCall epicsRingBytesUsedBytes(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;
    int nextGet, nextPut;
    int used;

    if (pring->lock) epicsSpinLock(pring->lock);
    nextGet = pring->nextGet;
    nextPut = pring->nextPut;
    if (pring->lock) epicsSpinUnlock(pring->lock);

    used = nextPut - nextGet;
    if (used < 0) used += pring->size;

    return used;
}

LIBCOM_API int epicsStdCall epicsRingBytesSize(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;

    return pring->size - SLOP;
}

LIBCOM_API int epicsStdCall epicsRingBytesIsEmpty(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;
    int isEmpty;

    if (pring->lock) epicsSpinLock(pring->lock);
    isEmpty = (pring->nextPut == pring->nextGet);
    if (pring->lock) epicsSpinUnlock(pring->lock);

    return isEmpty;
}

LIBCOM_API int epicsStdCall epicsRingBytesIsFull(epicsRingBytesId id)
{
    return (epicsRingBytesFreeBytes(id) <= 0);
}

LIBCOM_API int epicsStdCall epicsRingBytesHighWaterMark(epicsRingBytesIdConst id)
{
    ringPvt *pring = (ringPvt *)id;
    return pring->highWaterMark;
}

LIBCOM_API void epicsStdCall epicsRingBytesResetHighWaterMark(epicsRingBytesId id)
{
    ringPvt *pring = (ringPvt *)id;
    int used;
    if (pring->lock) epicsSpinLock(pring->lock);
    used = pring->nextGet - pring->nextPut;
    if (used < 0) used += pring->size;
    pring->highWaterMark = used;
    if (pring->lock) epicsSpinUnlock(pring->lock);
}
