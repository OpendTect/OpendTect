#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define od packages
#

if ( WIN32 OR ${PACKAGE_TYPE} STREQUAL "Devel" )
    list( APPEND PACKAGELIST develdefs develguidefs )
endif()

if ( ${PACKAGE_TYPE} STREQUAL "Production" )
    list( APPEND PACKAGELIST basedatadefs basedefs baseguidefs )
    if ( BUILD_DOCUMENTATION AND EXISTS "${CLASSDOC_SCRIPT_LOCATION}" )
	list( APPEND PACKAGELIST classdocdefs )
    endif()
    if ( BUILD_USERDOC AND EXISTS "${USERDOC_PROJECT}" )
	list( APPEND PACKAGELIST docdefs )
    endif()
endif()

if ( WIN32 )
    list( APPEND PACKAGELIST colopdefs )
endif()
