#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define od packages
# Author:       Nageswara
# Date:         May 2013

set( BASEPACKAGES basedatadefs )
set( PACKAGELIST basedefs develdefs )
if ( WIN32 )
    if ( "${BUILD_USERDOC}" STREQUAL "YES" AND EXISTS ${USERDOC_PROJECT} )
	set( PACKAGELIST ${PACKAGELIST} doc )
    endif()
endif()

if ( UNIX )
    if ( "${BUILD_DOCUMENTATION}" STREQUAL "ON" )
	set( PACKAGELIST ${PACKAGELIST} classdoc )
    endif()
endif()
