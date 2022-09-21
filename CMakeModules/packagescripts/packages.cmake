#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define od packages
# Author:       Nageswara
# Date:         May 2013

set( BASEPACKAGES basedatadefs )
set( PACKAGELIST basedefs develdefs )
if ( BUILD_USERDOC AND EXISTS "${USERDOC_PROJECT}" )
    list( APPEND PACKAGELIST doc )
endif()

if ( BUILD_DOCUMENTATION )
    list( APPEND PACKAGELIST classdoc )
endif()
