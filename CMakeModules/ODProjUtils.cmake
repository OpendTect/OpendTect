#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	May 2017	R. Singh
#_______________________________________________________________________________

macro( OD_CONF_PROJ4 )
    if( WIN32 )
	SET( CMAKE_C_FLAGS "/WX-" )
    elseif ( UNIX )
	SET( CMAKE_C_FLAGS "-w" )
    endif()
    if ( NOT EXISTS "${PROJ4_DIR}" )
	file(MAKE_DIRECTORY "${PROJ4_DIR}")
    endif()
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	${EXTGENERATOR}
	${EXTPLFARCH}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
	-DCMAKE_DEBUG_POSTFIX=d
	-DCMAKE_INSTALL_PREFIX=${PROJ4_DIR}/inst
	-DCMAKE_INSTALL_LIBDIR=lib
	-DCMAKE_SKIP_INSTALL_RPATH=ON
	-DBUILD_CCT=OFF
	-DBUILD_CS2CS=OFF
	-DBUILD_GEOD=OFF
	-DBUILD_GIE=OFF
	-DBUILD_NAD2BIN=OFF
	-DPROJ_TESTS=OFF
	"${OpendTect_DIR}/external/proj4"
	WORKING_DIRECTORY "${PROJ4_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( FATAL_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_CONF_PROJ4)

macro( OD_BUILD_PROJ4 )
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	--build "${PROJ4_DIR}"
	--config ${CMAKE_BUILD_TYPE}
	--clean-first
	--target install
	WORKING_DIRECTORY "${PROJ4_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( FATAL_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_BUILD_PROJ4)

macro( OD_ADD_PROJ )

    if ( DEFINED PROJ_DIR AND EXISTS "${PROJ_DIR}" )
	set( PROJ_NAME "PROJ" )
	set ( PROJ_DIR "${PROJ_DIR}/lib/cmake/proj" )
	find_package( ${PROJ_NAME} 9 REQUIRED )
    endif()

    if ( NOT PROJ_FOUND )
	set( PROJ_NAME "PROJ4" )
	set ( PROJ4_DIR "${OD_BINARY_BASEDIR}/external/proj4" )
	if ( NOT EXISTS "${PROJ4_DIR}/CMakeCache.txt" )
	    OD_CONF_PROJ4()
	    OD_BUILD_PROJ4()
	elseif ( NOT EXISTS "${PROJ4_DIR}/inst" )
	    OD_BUILD_PROJ4()
	endif()

	set( PROJ4_DIR "${PROJ4_DIR}/inst" )
	if ( WIN32 )
	    set ( PROJ4_DIR "${PROJ4_DIR}/local" )
	endif()
	set ( PROJ4_DIR "${PROJ4_DIR}/lib/cmake/proj4" )
	find_package( ${PROJ_NAME} 5  REQUIRED)

	if ( NOT ${PROJ_NAME}_FOUND )
	    message( FATAL_ERROR "Proj4 external not found" )
	endif()
    endif()


endmacro(OD_ADD_PROJ)

macro(OD_SETUP_PROJ)

    if ( OD_USEPROJ )
	if ( EXISTS "${${PROJ_NAME}_INCLUDE_DIRS}" )
	    list( APPEND OD_MODULE_INCLUDESYSPATH
		    ${${PROJ_NAME}_INCLUDE_DIRS} )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ${${PROJ_NAME}_LIBRARIES} )
	endif()
    endif( OD_USEPROJ )

endmacro(OD_SETUP_PROJ)
