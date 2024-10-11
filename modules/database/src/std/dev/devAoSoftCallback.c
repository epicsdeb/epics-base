/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* SPDX-License-Identifier: EPICS
* EPICS Base is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/* devAoSoftCallbackCallback.c */
/*
 *      Author:  Marty Kraimer
 *      Date:    04NOV2003
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "dbCa.h"
#include "link.h"
#include "special.h"
#include "aoRecord.h"
#include "epicsExport.h"

/* Create the dset for devAoSoftCallback */
static long write_ao(aoRecord *prec);

aodset devAoSoftCallback = {
    {6, NULL, NULL, NULL, NULL},
    write_ao, NULL
};
epicsExportAddress(dset, devAoSoftCallback);

static long write_ao(aoRecord *prec)
{
    struct link *plink = &prec->out;
    long status;

    if (prec->pact)
        return 0;

    status = dbPutLinkAsync(plink, DBR_DOUBLE, &prec->oval, 1);
    if (!status)
        prec->pact = TRUE;
    else if (status == S_db_noLSET)
        status = dbPutLink(plink, DBR_DOUBLE, &prec->oval, 1);

    return status;
}

