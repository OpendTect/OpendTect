#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set ( ITTNOTIFY_DIR "" CACHE PATH "Directory of ITT notify" )

if ( EXISTS ${ITTNOTIFY_DIR} )
    find_package( IttNotify )
    if ( ITTNOTIFY_FOUND )
	add_definitions( -D__ittnotify__ )
    endif ( ITTNOTIFY_FOUND )
endif ()
