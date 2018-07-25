#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	May 2017	R. Singh
#_______________________________________________________________________________

macro( OD_ADD_PROJ4 )

    set ( PROJ4_INSTDIR_LIB_DEBUG ${OD_LIB_INSTALL_PATH_DEBUG} )
    set ( PROJ4_INSTDIR_LIB_RELEASE ${OD_LIB_INSTALL_PATH_RELEASE} )

    if ( UNIX )
	SET( CMAKE_C_FLAGS "-w " )
    elseif( WIN32 )
	SET( CMAKE_C_FLAGS "/W0 /WX- " )
    endif()

    # Disable modules that are not needed by OD:
    option ( BUILD_CCT "Build CCT module" OFF )
    option ( BUILD_CS2CS "Build CS2CS module" OFF )
    option ( BUILD_GEOD "Build GEOD module" OFF )
    option ( BUILD_GIE "Build GIE module" OFF )
    option ( BUILD_NAD2BIN "Build NAD2BIN module" OFF )
    option ( PROJ_TESTS "Build Proj tests" OFF )

    add_subdirectory( ${CMAKE_SOURCE_DIR}/external/proj4 
		  ${CMAKE_BINARY_DIR}/external/proj4 )

    set ( PROJ4_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/proj4/src
			     ${CMAKE_BINARY_DIR}/external/proj4/src )
    set ( PROJ4_LIBRARY_PATH ${CMAKE_BINARY_DIR}/external/proj4/lib)
endmacro()

macro(OD_SETUP_PROJ4)

    if ( OD_USEPROJ4 )

	if ( NOT DEFINED PROJ4_INCLUDE_DIR )
	    OD_ADD_PROJ4()
	endif()

	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${PROJ4_INCLUDE_DIR} )

	list ( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${PROJ4_LIBRARY_PATH} )

	set( LIB proj )
	    if ( MSVC AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
		set( LIB ${LIB}_d ) # Proj4 uses _d as DEBUG_POSTFIX on MSVC
	endif()

	    file ( STRINGS ${CMAKE_SOURCE_DIR}/external/proj4/CMakeLists.txt
		   PROJ4_CMAKELIST )
	    foreach ( line ${PROJ4_CMAKELIST} )
		if ( ${line} MATCHES "PROJ_API_VERSION" )
		    string ( REGEX MATCH "[0-9.]+" PROJ_API_VERSION ${line} )
		endif() 
		if ( ${line} MATCHES "PROJ_BUILD_VERSION" )
		    string ( REGEX MATCH "[0-9.]+" PROJ_BUILD_VERSION ${line} )
		endif() 
	    endforeach()

	if( UNIX OR APPLE )
	    if( APPLE )
		#installing Mac proj4 files(like libproj.12.dylib etc. )
		set( PROJ4LIBBUILDVER ${PROJ4_LIBRARY_PATH}/lib${LIB}.${PROJ_BUILD_VERSION}.dylib )
		set( PROJ4LIB lib${LIB}.${PROJ_API_VERSION}.dylib)

	    else()
		#installing linux proj4 files(like libproj.so.12 etc. )
		set( PROJ4LIBBUILDVER ${PROJ4_LIBRARY_PATH}/lib${LIB}.so.${PROJ_BUILD_VERSION} )
		set( PROJ4LIB lib${LIB}.so.${PROJ_API_VERSION})
	    endif()
	    if ( ${CMAKE_BUILD_TYPE} STREQUAL "Release" )
		install( PROGRAMS ${PROJ4LIBBUILDVER} DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
			 RENAME ${PROJ4LIB} CONFIGURATIONS ${CMAKE_BUILD_TYPE} )
	    else()
		install( PROGRAMS ${PROJ4LIBBUILDVER} DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}
			 RENAME ${PROJ4LIB} CONFIGURATIONS ${CMAKE_BUILD_TYPE} )
	    endif()
	    set( PROJ4LIB ${PROJ4LIB} PARENT_SCOPE)
	endif()
    endif()

    list( APPEND OD_MODULE_EXTERNAL_LIBS proj )

endmacro(OD_SETUP_PROJ4)
