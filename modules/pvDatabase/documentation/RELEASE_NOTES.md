# pvDatabaseCPP Module

This document summarizes the changes to the module between releases.

## Release 4.7.0 (EPICS 7.0.7, Sep 2022)

* Added support for the whole structure (master field) server side plugins.
  The whole structure is identified as the `_` string, and a pvRequest string
  that applies a plugin to it takes the form:

  `field(_[XYZ=A:3;B:uniqueId])`

  where `XYZ` is the name of a specific filter plugin that takes parameters
  `A` and `B` with values `3` and `uniqueId` respectively.

## Release 4.6.0 (EPICS 7.0.6, Jul 2021)

* Access Security is now supported.
* <b>special</b> has been revised and extended.
* addRecord, removeRecord, processRecord, and traceRecord are replaced by pvdbcr versions.
* <b>support</b> is DEPRECATED

## Release 4.5.3 (EPICS 7.0.5, Feb 2021)

* The previously deprecated destroy methods have been removed.
  Any application code that was previously calling these can just remove
  those calls.

## Release 4.5.2 (EPICS 7.0.3.2, May 2020)

* plugin support is new
* fixed issues #53 and #52

## Release 4.5.1 (EPICS 7.0.3.1, Nov 2019)

* addRecord is new.
* Doxygen updates and read-the-docs integration.


## Release 4.5.0 (EPICS 7.0.3, Jul 2019)

* support is a new feature.
* processRecord is new.


## Release 4.4.2 (EPICS 7.0.2.2, Apr 2019)

Formerly if a client makes a request for a subfield of a non structure field
it resulted in a crash.

Now if a request is made for a subfield of a non structure field

1) if the field is not a union an exception is thrown which is passed to the client.
2) if the field is a union
    a) if more than one subfield is requested an exception is thrown 
    b) if the subfield is the type for the current union the request succeeds
    c) if type is not the same an exception is thrown


## Release 4.4.1 (EPICS 7.0.2.1, Mar 2019)

* Cleaned up some build warnings.
* RTEMS test harness simplified.


## Release 4.4 (EPICS 7.0.2, Dec 2018)

* pvCopy is now implemented in pvDatabaseCPP. The version in pvDatacPP can be deprecated.
* plugin support is implemented.


## Release 4.3 (EPICS 7.0.1, Dec 2017)

* Updates for pvAccess API and build system changes.


## Release 4.2 (EPICS V4.6, Aug 2016)

* The examples are moved to exampleCPP
* Support for channelRPC is now available.
* removeRecord and traceRecord are now available.

The test is now a regression test which can be run using:

     make runtests


## Release 4.1 (EPICS V4.5, Oct 2015)

This is the first release of pvDatabaseCPP.

It provides functionality equivalent to pvDatabaseJava.
