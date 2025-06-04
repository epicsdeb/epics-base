#!../../bin/linux-x86_64-debug/softIocPVA

# Normal IOC executables will be linked against libcaputlog
# at built time.   Because pva2pva is usually built before
# caputlog, softIocPVA can't.  Instead load dynamically.
#
# Requires a target and configuration w/ dynamic linking support.
#
# registerAllRecordDeviceDrivers added in Base >= 7.0.5

< envPaths
# or
#epicsEnvSet("CAPUTLOG", "/path/to/caputlog")

dlload $(CAPUTLOG)/lib/$(ARCH)/libcaPutLog.so
dbLoadDatabase $(CAPUTLOG)/dbd/caPutLog.dbd
registerAllRecordDeviceDrivers

dbLoadRecords("putlog.db","P=TST:")

asSetFilename("$(PWD)/putlog.acf")
asSetSubstitutions("USER=$(USER)")
asInit

var caPutLogDebug 5
caPutLogInit localhost:3456

iocInit()


# concurrently run:
##   nc -l -p 3456

# Then try:
##   pvput TST:A 4
