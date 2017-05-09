#_______________________Pmake___________________________________________________
#
#	Makefile :	CRS
#	May 2017	Raman
#	RCS :		$Id$
#_______________________________________________________________________________

option( BUILD_CRS "Build the CRS/uiCRS plugins" OFF )
if ( BUILD_CRS )
    set( PROJ4_DIR "" CACHE PATH "Directory containing Proj4 Installation" )
    if ( NOT EXISTS ${PROJ4_DIR}/include/proj_api.h )
	message( FATAL_ERROR "PROJ4_DIR is not set or not valid" )
    endif()

    list ( APPEND OD_MODULE_INCLUDESYSPATH ${PROJ4_DIR}/include )
    if ( UNIX )
	if ( NOT APPLE )
	    list(APPEND OD_MODULE_EXTERNAL_LIBS ${PROJ4_DIR}/lib/libproj.so )
	endif()
    elseif ( WIN32 )
	list(APPEND OD_MODULE_EXTERNAL_LIBS ${PROJ4_DIR}/lib/libproj.lib )
    endif ()

    set( OD_MODULE_DEPS Basic)
    set( OD_IS_PLUGIN yes)
    set( OD_MODULE_SOURCES
		crsproj.cc
		crssystem.cc
		crspi.cc )

    OD_INIT_MODULE()
endif()
