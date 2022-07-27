#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	May 2017	R. Singh
#_______________________________________________________________________________


macro( OD_CONF_PROJ )
    if( WIN32 )
	SET( CMAKE_C_FLAGS "/WX-" )
    elseif ( UNIX )
	SET( CMAKE_C_FLAGS "-w" )
    endif()
    if ( NOT EXISTS "${PROJ_DIR}" )
	message( FATAL_ERROR "PROJ_DIR ${PROJ_DIR} does not exist" )
    endif()

    if ( NOT DEFINED SQLITE_DIR )
	SET( SQLITE_DIR "" )
    endif()

    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	${EXTGENERATOR}
	${EXTPLFARCH}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
	-DCMAKE_DEBUG_POSTFIX=d
	-DCMAKE_INSTALL_PREFIX=${PROJ_DIR}/inst
	-DCMAKE_PREFIX_PATH=${SQLITE_DIR}
	-DCMAKE_INSTALL_LIBDIR=lib
	-DCMAKE_SKIP_INSTALL_RPATH=ON
	-DBUILD_APPS=OFF
	-DBUILD_CCT=ON
	-DBUILD_CS2CS=ON
	-DBUILD_PROJ=ON
	-DBUILD_PROJINFO=ON
	-DBUILD_TESTING=OFF
	-DENABLE_CURL=OFF
	-DENABLE_TIFF=OFF
	"${OpendTect_DIR}/external/proj"
	WORKING_DIRECTORY "${PROJ_DIR}"
	ERROR_VARIABLE ERROUTPUT
	OUTPUT_VARIABLE OUTPUTVAR
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	string(REGEX MATCH "sqlite[1-9] >= [1-9]\.[0-9]+ required"
		SQLITESTR ${ERROUTPUT} )
	if ( NOT ${SQLITESTR} STREQUAL "" )
	    SET( ERROUTPUT "Failed to configure PROJ : ${SQLITESTR}\n"
		     "Please provide SQLITE_DIR\n${ERROUTPUT}" )
	endif()
	message( FATAL_ERROR ${ERROUTPUT} )
    endif()
endmacro(OD_CONF_PROJ)

macro( OD_BUILD_PROJ )
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	--build "${PROJ_DIR}"
	--config ${CMAKE_BUILD_TYPE}
	--clean-first
	--target install
	WORKING_DIRECTORY "${PROJ_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( FATAL_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_BUILD_PROJ)


macro( OD_ADD_PROJ )
    if ( OD_NO_PROJ )
	add_definitions( -DOD_NO_PROJ )
    else()
	if ( BUILD_PROJ )
	    set ( PROJ_DIR "${OD_BINARY_BASEDIR}/external/proj" )
	    if ( NOT EXISTS "${PROJ_DIR}/CMakeCache.txt" )
		OD_CONF_PROJ()
		OD_BUILD_PROJ()
	    elseif ( NOT EXISTS "${PROJ_DIR}/inst" )
		OD_BUILD_PROJ()
	    endif()

	    set( PROJ_DIR "${PROJ_DIR}/inst" )
	    if ( WIN32 )
		set ( PROJ_DIR "${PROJ_DIR}/local" )
	    endif()
	endif()

	if ( NOT EXISTS "${PROJ_DIR}/proj-config.cmake" )
	    set ( PROJ_DIR "${PROJ_DIR}/lib/cmake/proj" )
	endif()

	find_package( PROJ QUIET )

	if ( NOT PROJ_FOUND )
	    message( FATAL_ERROR "Proj not found" )
	endif()
    endif()

endmacro(OD_ADD_PROJ)

macro(OD_SETUP_PROJ)

    if ( OD_USEPROJ )
	if ( EXISTS "${PROJ_INCLUDE_DIRS}" )
	    list( APPEND OD_MODULE_INCLUDESYSPATH
		    "${PROJ_INCLUDE_DIRS}" )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS "${PROJ_LIBRARIES}" )
	endif()

	install( FILES "${PROJ_INCLUDE_DIRS}/../share/proj/proj.db"
		 DESTINATION data/CRS )
	install( FILES "${PROJ_INCLUDE_DIRS}/../share/doc/proj/COPYING"
		 DESTINATION data/CRS )

    endif( OD_USEPROJ )

endmacro(OD_SETUP_PROJ)
