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
ifneq (\$(SKIP_HARDENING),YES)
ifeq (\$(OS_CLASS),Linux)

TARGET_CPPFLAGS = $CPPFLAGS
TARGET_CFLAGS = $CFLAGS
TARGET_CXXFLAGS = $CXXFLAGS
TARGET_LDFLAGS = $LDFLAGS

endif
endif
EOF
