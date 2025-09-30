#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define classdoc package variables
#

file( GLOB CLASSDOCFILES RELATIVE "${COPYFROMDIR}/doc/Programmer/Generated"
	"${COPYFROMDIR}/doc/Programmer/Generated/*" )
foreach( FNM ${CLASSDOCFILES} )
    list( APPEND OTHERFILES "${COPYFROMDIR}/doc/Programmer/Generated/${FNM}" )
    list( APPEND OTHERFILESDEST "${COPYTODIR}/doc/Programmer" )
endforeach()

set( ISCLASSDOC TRUE )

set( PACK classdoc )
