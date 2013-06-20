#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	June 2013	K. Tingdahl
#	RCS :		$Id: ODIttNotifyUtils.cmake 30343 2013-06-18 06:54:22Z kristofer.tingdahl@dgbes.com $
#_______________________________________________________________________________

if ( ITTNOTIFY_DIR )
    find_package( IttNotify )
    if ( ITTNOTIFY_FOUND )
	add_definitions( -D__ittnotify__ )
    endif ( ITTNOTIFY_FOUND )
endif ( ITTNOTIFY_DIR )
