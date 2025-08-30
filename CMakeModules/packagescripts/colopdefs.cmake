#________________________________________________________________________
#
# Copyright:	dGB Beheer B.V.
# License:	https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define uiCOLOP package variables
#

set( PLUGINS uiCOLOP )
set( OTHERFILES "C:/appman/apps/colop/COLOP.exe" )
set( OTHERFILESDEST "${COPYTOLOCDIR}" )
set( PACK colop )

if ( ${PACKAGE_TYPE} STREQUAL "Devel" )
    set( PACK "${PACK}devel" )
endif()
