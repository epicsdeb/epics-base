# pvDatabaseCPP

The EPICS **pvDatabase** module provides a set of network accessible, smart, memory resident records with a C++ API. Each record has data composed of a top level PVStructure. Each record has a name which is the channelName for pvAccess. A local Channel Provider implements the complete ChannelProvider and Channel interfaces as defined by pvAccess. The local provider gives access to the records in the pvDatabase. This local provider is accessed by the remote pvAccess server. A record is smart because code can be attached to a record, which is accessed via a method named process.

The pvDatabase module implements a synchronous C++ server interface to pvAccessCPP that was designed to be easier to use than the basic pvAccess server API.

## Links

- General information about EPICS can be found at the
  [EPICS Controls website](https://epics-controls.org).
- API documentation for this module can be found in its
  documentation directory, in particular the file
  pvDatabaseCPP.html

## Building

This module is included as a submodule of a full EPICS 7 release and will be compiled during builds of that software.

## Examples

Some examples are available in the separate exampleCPP module.
