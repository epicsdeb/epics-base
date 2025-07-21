#ifndef PVA2PVA_H
#define PVA2PVA_H

#include <epicsGuard.h>

#include <pv/pvAccess.h>

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

void registerGWClientIocsh();
void gwServerShutdown();
void gwClientShutdown();
void registerReadOnly();

#endif // PVA2PVA_H
