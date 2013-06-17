#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	June 2013	K. Tingdahl
#	RCS :		$Id: ODZlibUtils.cmake 29376 2013-04-19 14:21:04Z kristofer.tingdahl@dgbes.com $
#_______________________________________________________________________________

if ( ITTNOTIFY_DIR )
    find_package( IttNotify )
    if ( ITTNOTIFY_FOUND )
	add_definitions( -D__ittnotify__ )
    endif ( ITTNOTIFY_FOUND )
endif ( ITTNOTIFY_DIR )
