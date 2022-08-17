#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define od packages
#

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
