#!/bin/sh
set -e

export DEB_CPPFLAGS_STRIP="-g -O2"
export DEB_CFLAGS_STRIP="-g -O2"
export DEB_CXXFLAGS_STRIP="-g -O2"


CPPFLAGS=`dpkg-buildflags --get CPPFLAGS`
CFLAGS=`dpkg-buildflags --get CFLAGS`
CXXFLAGS=`dpkg-buildflags --get CXXFLAGS`
LDFLAGS=`dpkg-buildflags --get LDFLAGS`

cat << EOF
# set SKIP_HARDENING=YES to disable
ifeq (\$(OS_CLASS),Linux)

HARDEN_CPPFLAGS = $CPPFLAGS
HARDEN_CFLAGS = $CFLAGS
HARDEN_CXXFLAGS = $CXXFLAGS
HARDEN_LDFLAGS = $LDFLAGS

endif
EOF
