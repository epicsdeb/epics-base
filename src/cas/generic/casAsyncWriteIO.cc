/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *      casAsyncWriteIO.cc,v 1.7.2.1 2003/08/01 19:13:34 soliday Exp
 *
 *      Author  Jeffrey O. Hill
 *              johill@lanl.gov
 *              505 665 1831
 */

#include <string>
#include <stdexcept>

#define epicsExportSharedSymbols
#include "casdef.h"
#include "casAsyncWriteIOI.h"

casAsyncWriteIO::casAsyncWriteIO ( const casCtx & ctx ) :
	pAsyncWriteIOI ( new casAsyncWriteIOI ( *this, ctx ) )
{
}

void casAsyncWriteIO::serverInitiatedDestroy ()
{
    this->pAsyncWriteIOI = 0;
    this->destroy ();
}

casAsyncWriteIO::~casAsyncWriteIO()
{
    if ( this->pAsyncWriteIOI ) {
        throw std::logic_error ( 
            "the server library *must* initiate asynchronous IO destroy" );
    }
}

caStatus casAsyncWriteIO::postIOCompletion ( caStatus completionStatusIn )
{
    if ( this->pAsyncWriteIOI ) {
	    return this->pAsyncWriteIOI->postIOCompletion ( completionStatusIn );
    }
    else {
        return S_cas_redundantPost;
    }
}

void casAsyncWriteIO::destroy ()
{
    delete this;
}
