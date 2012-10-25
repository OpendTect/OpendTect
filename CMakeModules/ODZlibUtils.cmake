#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( QT_QTCORE_INCLUDE_DIR )
    if ( EXISTS "${QT_QTCORE_INCLUDE_DIR}/zlib.h" )
        set ( ZLIB_INCLUDE_DIR ${QT_QTCORE_INCLUDE_DIR} )
	set ( ZLIB_LIBRARY ${QT_QTCORE_LIBRARY} )
    endif()
endif()

if ( NOT DEFINED ZLIB_INCLUDE_DIR )
    find_package( Zlib REQUIRED )
endif()
