#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define doc package variables
#

list( APPEND OTHERFILES "${COPYFROMDIR}/doc/od_userdoc" )
list( APPEND OTHERFILESDEST "${COPYTODIR}/doc" )

set( ISUSERDOC TRUE )

set( PACK doc )
