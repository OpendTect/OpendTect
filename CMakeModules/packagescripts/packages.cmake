#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define od packages
#

set( BASEPACKAGES basedatadefs )
if( ${OD_PLFSUBDIR} STREQUAL "lux64" )
    set( PACKAGELIST odbatchdefs )
else()
    set( PACKAGELIST basedefs develdefs )
endif()

if ( BUILD_USERDOC AND EXISTS "${USERDOC_PROJECT}" )
    list( APPEND PACKAGELIST doc )
endif()

if ( BUILD_DOCUMENTATION )
    list( APPEND PACKAGELIST classdoc )
endif()
