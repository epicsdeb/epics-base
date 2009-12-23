#!/usr/bin/perl
#*************************************************************************
# Copyright (c) 2008 The University of Chicago, as Operator of Argonne
#     National Laboratory.
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution.
#*************************************************************************

# fullPathName.pl,v 1.1.2.3 2008/09/23 22:13:50 anj Exp

# Determines an absolute pathname for its argument,
# which may be either a relative or absolute path and
# might have trailing parts that don't exist yet.

use strict;

use FindBin qw($Bin);
use lib "$Bin/../../lib/perl";

use EPICS::Path;

print AbsPath(shift), "\n";

