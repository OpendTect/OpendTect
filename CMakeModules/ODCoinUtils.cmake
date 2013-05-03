#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set(COINDIR "" CACHE PATH "COIN Location" )

macro(OD_SETUP_COIN)
    if ( (NOT DEFINED COINDIR) OR COINDIR STREQUAL "" )
	message( FATAL_ERROR "COINDIR not set")
    endif()

    find_package( OpenGL )

    if (WIN32)
	find_library(COINLIB_RELEASE NAMES Coin3 PATHS ${COINDIR}/lib )
	find_library(SOQTLIB_RELEASE NAMES SoQt1 PATHS ${COINDIR}/lib )
	find_library(SIMAGELIB_RELEASE NAMES simage1 PATHS ${COINDIR}/lib )
	find_library(COINLIB_DEBUG NAMES Coin3d PATHS ${COINDIR}/lib )
	find_library(SOQTLIB_DEBUG NAMES SoQt1d PATHS ${COINDIR}/lib )
	find_library(SIMAGELIB_DEBUG NAMES simage1d PATHS ${COINDIR}/lib )
    else()
	find_library(COINLIB_RELEASE NAMES Coin PATHS ${COINDIR}/lib )
	find_library(SOQTLIB_RELEASE NAMES SoQt PATHS ${COINDIR}/lib )
	find_library(SIMAGELIB_RELEASE NAMES simage PATHS ${COINDIR}/lib )
	find_library(COINLIB_DEBUG NAMES Coind PATHS ${COINDIR}/lib )
	find_library(SOQTLIB_DEBUG NAMES SoQtd PATHS ${COINDIR}/lib )
	find_library(SIMAGELIB_DEBUG NAMES simaged PATHS ${COINDIR}/lib )
    endif()

    OD_MERGE_LIBVAR( COINLIB )
    OD_MERGE_LIBVAR( SOQTLIB )
    OD_MERGE_LIBVAR( SIMAGELIB )

    if (WIN32)
	find_library(OD_SIMVOLEON_LIBRARY_RELEASE NAMES SimVoleon2 PATHS ${COINDIR}/lib )
	find_library(OD_SIMVOLEON_LIBRARY_DEBUG NAMES SimVoleon2d PATHS ${COINDIR}/lib )
	OD_MERGE_LIBVAR( OD_SIMVOLEON_LIBRARY )
    else()
	set(TMPVAR ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${OD_STATIC_EXTENSION})
	find_library(OD_SIMVOLEON_LIBRARY NAMES SimVoleon
		     PATHS ${COINDIR}/lib REQUIRED )
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${TMPVAR})
    endif()

    if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	foreach ( BUILD_TYPE "Debug" "Release" )

	    set( CARGS ${COINLIB} ${SOQTLIB} ${SIMAGELIB} )
	    if ( WIN32 )
		set( CARGS ${CARGS} ${OD_SIMVOLEON_LIBRARY} )
	    endif()

	    OD_FILTER_LIBRARIES( CARGS ${BUILD_TYPE} )
	    unset( ARGS )

	    foreach ( LIB ${CARGS} )
		get_filename_component( COINLIBNAME ${LIB} NAME_WE )
		if ( UNIX OR APPLE )
		    if ( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
			file( GLOB ALLLIBS "${COINDIR}/lib/${COINLIBNAME}.so.[0-9][0-9]" )
		    elseif( APPLE )
			file( GLOB ALLLIBS "${COINDIR}/lib/${COINLIBNAME}.[0-9][0-9].dylib" )
		    endif()
		    list( APPEND ARGS ${ALLLIBS} )
		    list( GET ARGS 0 FILENM )
		    OD_INSTALL_LIBRARY( ${FILENM} )
		    set( ARGS )
		    set( ALLLIBS "" )
		elseif( WIN32 )
		    list( APPEND ARGS ${COINDIR}/bin/${COINLIBNAME}.dll )
		endif()
	    endforeach()

	    if ( WIN32 )
		install( PROGRAMS ${ARGS}
			DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${BUILD_TYPE} )
	    endif()
	endforeach()
    endif()

    if (OD_USECOIN)
	if ( OD_EXTRA_COINFLAGS )
	    add_definitions( ${OD_EXTRA_COINFLAGS} )
	endif( OD_EXTRA_COINFLAGS )

	list(APPEND OD_MODULE_INCLUDESYSPATH ${COINDIR}/include )
	set(OD_COIN_LIBS ${COINLIB} ${SOQTLIB} ${OPENGL_gl_LIBRARY} )
    endif()

    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_COIN_LIBS} )
endmacro(OD_SETUP_COIN)


