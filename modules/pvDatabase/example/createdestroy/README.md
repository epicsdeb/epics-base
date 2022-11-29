# pvDatabaseCPP/example/createdestroy

This is an example that:

1) Gets the master PVDatabase
2) Create ChannelProviderLocal
3) Creates a ServerContext

Then it executes a forever loop that:

1) creates a pvRecord and adds it to the pvDatabase.
2) creates a pvac::ClientProvider
3) creates a pvac::ClientChannel
4) creates a monitor on the channel
5) runs a loop 10 times that: does a put to the channel, and then gets the data for any outstanding monitors
6) removes the pvRecord from the pvDatabase

It also has options to set trace level for the pvRecord and to periodically pause by asking for input.


