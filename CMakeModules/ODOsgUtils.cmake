#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_OSGDIR )
    if ( NOT DEFINED OSG_DIR )
	if ( DEFINED OSG_LIBRARY_RELEASE AND EXISTS ${OSG_LIBRARY_RELEASE} )
	    get_filename_component( OSG_DIR ${OSG_LIBRARY_RELEASE} DIRECTORY )
	elseif ( DEFINED OSG_LIBRARY_DEBUG AND EXISTS ${OSG_LIBRARY_DEBUG} )
	    get_filename_component( OSG_DIR ${OSG_LIBRARY_DEBUG} DIRECTORY )
	elseif ( DEFINED OSG_LIBRARY AND EXISTS ${OSG_LIBRARY} )
	    get_filename_component( OSG_DIR ${OSG_LIBRARY} DIRECTORY )
	else()
	    set( OSG_DIR "" CACHE PATH "OSG Location" )
	    message( FATAL_ERROR "OSG_DIR is not defined" )
	endif()
	get_filename_component( OSG_DIR ${OSG_DIR} DIRECTORY )
    endif()
endmacro(OD_FIND_OSGDIR)

macro( OD_CONF_OSGGEO )
    if ( NOT EXISTS "${OSGGEO_DIR}" )
	file(MAKE_DIRECTORY "${OSGGEO_DIR}")
    endif()
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	${EXTGENERATOR}
	${EXTPLFARCH}
	${EXTPLFTOOLSET}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_CXX_FLAGS=${CMAKE_EXT_CXX_FLAGS}
	-DQTDIR=${QTDIR}
	-DOSG_DIR=${OSG_DIR}
	-DOSGGEO_LIB_POSTFIX=
	-DOSGGEO_USE_DEBUG_OSG=ON
	-DBUILD_EXAMPLES=OFF
	-DCMAKE_INSTALL_PREFIX=${OSGGEO_DIR}/inst
	-DCMAKE_SKIP_INSTALL_RPATH=ON
	"${CMAKE_SOURCE_DIR}/external/osgGeo"
	WORKING_DIRECTORY "${OSGGEO_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( SEND_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_CONF_OSGGEO)

macro( OD_BUILD_OSGGEO )
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	--build "${OSGGEO_DIR}"
	--config ${CMAKE_BUILD_TYPE}
	--clean-first
	--target install
	WORKING_DIRECTORY "${OSGGEO_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( SEND_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_BUILD_OSGGEO)

macro( OD_ADD_OSG )
    if ( OD_NO_OSG )
	add_definitions( -DOD_NO_OSG )
    else()
	OD_FIND_OSGDIR()

	set( OSGGEO_DIR "${OD_BINARY_BASEDIR}/external/osgGeo" )
	if ( NOT EXISTS "${OSGGEO_DIR}/CMakeCache.txt" )
	    OD_CONF_OSGGEO()
	    OD_BUILD_OSGGEO()
	elseif ( NOT EXISTS "${OSGGEO_DIR}/inst" )
	    OD_BUILD_OSGGEO()
	endif()

	set(ENV{OSG_DIR} "${OSG_DIR}")

        if ( APPLE )
	    set( OSGGEO_DIR "${OSGGEO_DIR}/inst/Contents/Frameworks" )
	    set( OSGGEO_INCLUDE_DIR "${OSGGEO_DIR}/../Resources/include" )
	    list( APPEND CMAKE_MODULE_PATH "${OSGGEO_DIR}/../Resources/share/CMakeModules" )
        else()
	    set( OSGGEO_DIR "${OSGGEO_DIR}/inst" )
	    set( OSGGEO_INCLUDE_DIR "${OSGGEO_DIR}/include" )
	    list( APPEND CMAKE_MODULE_PATH "${OSGGEO_DIR}/share/CMakeModules" )
        endif()
	cmake_policy(SET CMP0017 NEW)

	find_package( OpenSceneGraph QUIET COMPONENTS osgDB osgGA osgUtil osgManipulator osgWidget osgViewer osgVolume osgText osgSim osgGeo )

	if ( NOT OSG_FOUND )
	    message( FATAL_ERROR "Cannot find/use the OSG installation" )
	elseif( NOT OSGGEO_FOUND )
	    message( FATAL_ERROR "OpenSceneGraph-osgGeo not found" )
	endif()

	unset( OSG_DIR CACHE )
    endif(OD_NO_OSG)

endmacro(OD_ADD_OSG)

macro(OD_SETUP_OSG)

    if( OD_USEOSG )
	if ( NOT EXISTS "${OSG_INCLUDE_DIR}" OR
	     NOT EXISTS "${OSGGEO_INCLUDE_DIR}" )
	    message( FATAL_ERROR "osg/osgGeo is NOT a valid target" )
	endif()

	list(APPEND OD_MODULE_INCLUDESYSPATH
		${OSGGEO_INCLUDE_DIR}
		${OSG_INCLUDE_DIR} )

	if ( OD_EXTRA_OSGFLAGS )
	    add_definitions( ${OD_EXTRA_OSGFLAGS} )
	endif ( OD_EXTRA_OSGFLAGS )

	set(OSGMODULES
		OSG
		OSGDB
		OSGGA
		OSGUTIL
		OSGMANIPULATOR
		OSGWIDGET
		OSGVIEWER
		OSGVOLUME
		OPENTHREADS
		OSGTEXT
		OSGSIM
		OSGGEO )

	foreach( OSGMODULE ${OSGMODULES} )
	    if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
		if ( ${OSGMODULE}_LIBRARY_RELEASE )
		    list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY_RELEASE} )
		elseif ( ${OSGMODULE}_LIBRARY_DEBUG )
		    list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY_DEBUG} )
		else()
		    message(FATAL_ERROR "${OSGMODULE}_LIBRARY is missing")
		endif()
	    elseif ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		if ( ${OSGMODULE}_LIBRARY_DEBUG )
		    list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY_DEBUG} )
		elseif ( ${OSGMODULE}_LIBRARY )
		    list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY} )
		else()
		    message(FATAL_ERROR "${OSGMODULE}_LIBRARY_DEBUG is missing")
		endif()
	    endif()
	endforeach()

	list ( APPEND OD_MODULE_EXTERNAL_LIBS ${OD_OSG_LIBS} )
	if ( UNIX )
	    add_fontconfig( OD_MODULE_EXTERNAL_LIBS "${OD_MODULE_EXTERNAL_LIBS}" )
	endif()
    endif()


endmacro(OD_SETUP_OSG)
