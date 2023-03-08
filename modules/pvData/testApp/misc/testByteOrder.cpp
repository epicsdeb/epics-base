/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#include <testMain.h>
#include <epicsEndian.h>
#include <stdio.h>

MAIN(testByteOrder)
{
printf("EPICS_BYTE_ORDER: %s\n", (EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE) ? "little" : "big");
printf("EPICS_FLOAT_WORD_ORDER: %s\n", (EPICS_FLOAT_WORD_ORDER == EPICS_ENDIAN_LITTLE) ? "little" : "big");
return 0;
}
