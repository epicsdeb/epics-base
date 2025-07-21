# normativeTypes Module

This document summarizes the changes to the module between releases.

## Release 6.0.1 (EPICS 7.0.3.1, October 2019)

* Doxygen updates and read-the-docs integration.

## Release 6.0.0 (EPICS 7.0.3, July 2019)

* Reimplement `isCompatible` methods to use a new internal validation API in order to make the implementation uniform, less repetitive and less strict.

The new implementation is less strict in the sense that it considers types that can be converted into one another compatible with each other. For example, any `Scalar` is considered compatible with any other `Scalar`, regardless of the underlying type. Normative Types users are advised to use `getAs` and `putFrom` when getting/putting data from/into `PVScalar`s and `PVScalarArray`s.

Also, `isCompatible` methods now disregard field order and extra fields that are not part of the specification.

This change is not expected to break any current server or client, but it will break existing clients that rely on the previous `isCompatible` strictness once servers start to take advantage of `isCompatible` now being less strict.

## Release 5.2.2

- Fix NTTable::getColumnNames().

## Release 5.2.1 (EPICS 7.0.2, Dec 2018)

* No functional changes.
* Removal of declaration for unimplemented PVNTField::createAlarmLimit() and elimination of unused variables.


## Release 5.2.0 (EPICS 7.0.1, Dec 2017)

This release contains bug fixes and minor source updates needed to
build against the latest version of pvData.


## Release 5.1.2 (EPICS V4.6, Aug 2016)

The main changes since release 5.1.1 are:

* NTUnionBuilder: Add missing value() function
* Updated document: Now document all Normative Types


## Release 5.1.1

The main changes since release 5.0 are:

* Linux shared library version added
* Headers and source locations have changed
* Missing is_a implementations added
* NTAttribute::addTags() is now non-virtual
* New license file replaces LICENSE and COPYRIGHT

### Shared library version added

Linux shared library version numbers have been added by setting SHRLIB_VERSION
(to 5.1 in this case). So shared object will be libnt.so.5.1 instead of
libpvData.so.

### Headers and source locations have changed

Source has moved out of nt directory directly into src.

Headers have been moved into a pv directory. This facilitates using some IDEs
such as Qt Creator.

src/nt/ntscalar.cpp -> src/ntscalar.cpp
src/nt/ntscalar.h   -> src/pv/ntscalar.h

### Missing is_a implementations added

is_a(PVStructurePtr const &) implementation has been added for each type.


## Release 5.0 (EPICS V4.5, Oct 2015)

This release adds support through wrapper classes and builders for the
remaining Normative Types:

* NTEnum
* NTMatrix
* NTURI
* NTAttribute
* NTContinuum
* NTHistogram
* NTAggregate
* NTUnion
* NTScalarMultiChannel

Release 5.0 therefore implements fully the
[16 Mar 2015 version](http://epics-pvdata.sourceforge.net/alpha/normativeTypes/normativeTypes_20150316.html)
 of the normativeTypes specification.

Each wrapper class has an extended API:

* is_a now has a convenience overload taking a PVStructure.
* isCompatible, reporting introspection type compatibility, now has an overload
  taking a Structure. The PVStructure version is retained as a convenience
  method and for backwards compatibility.
* An isValid function now reports validity of a compatible PVStructure's data
  with respect to the specification.

Other changes are:

* Support for NTAttributes extended as required by NTNDArray
  (NTNDArrayAttributes).
* A new class for parsing NT IDs (NTID).
* Resolution of the confusion between column names and labels in NTTable and
  improved API. Function for adding columns is now addColumn rather than add.
  New getColumnNames function provided.
* isConnected is treated as an optional rather than a required field in
  NTMultiChannelArray. isConnected() and addIsConnected() functions added to
  wrapper and builder respectively.
* Unit tests for all new classes.

## Release 4.0 (EPICS V4.4, Dec 2014)

This is the first release of normativeTypesCPP that is part of an official
EPICS V4 release.
It is a major rewrite of the previous versions of normativeTypesCPP.

This release provides support through wrapper classes and builders for the
following Normative Types:

* NTScalar
* NTScalarArray
* NTNameValue
* NTTable
* NTMultiChannel
* NTNDArray

Each type has a wrapper class of the same name which has functions for checking 
compatibility of existing PVStructures (isCompatible) and the reported types of 
Structures (is_a), wraps existing PVStructures (wrap, wrapUnsafe) and provides
a convenient interface to all required and optional fields.

Each type has a builder which can create a Structure, a PVStructure or a
wrapper around a new PVStructure. In each case optional or extra fields can be
added and options such as choice of scalar type can be made.


Additional features are:

* Utility classes NTField and NTPVField for standard structure fields and
  NTUtils for type IDs.
* Unit tests for the implemented classes.

