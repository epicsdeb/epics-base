#!/usr/bin/env perl
#*************************************************************************
# SPDX-License-Identifier: EPICS
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution.
#*************************************************************************
#
# Use MS Visual C++ compiler version number to determine if
# we want to use the Manifest Tool (status=1) or not (status=0)
#
# VC compiler versions >= 14.00 will have status=1
# VC compiler versions 10.00 - 13.10 will have status=0
# EPICS builds with older VC compilers is not supported
#

my $versionString=`cl 2>&1`;

if ($versionString =~ m/Version 16./) {
 $status=0;
} elsif ($versionString =~ m/Version 15./){
 $status=1;
} elsif ($versionString =~ m/Version 14./){
 $status=1;
} elsif ($versionString =~ m/Version 13.10/){
 $status=0;
} elsif ($versionString =~ m/Version 13.0/){
 $status=0;
} elsif ($versionString =~ m/Version 12./){
 $status=0;
} elsif ($versionString =~ m/Version 11./){
 $status=0;
} elsif ($versionString =~ m/Version 10./){
 $status=0;
} else {
 $status=0;
}
print "$status\n";
exit;
