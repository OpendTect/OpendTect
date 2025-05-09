#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_OSGDIR )
    if ( NOT DEFINED OSG_ROOT )
	if ( DEFINED OSG_LIBRARY_RELEASE AND EXISTS ${OSG_LIBRARY_RELEASE} )
	    get_filename_component( OSG_ROOT ${OSG_LIBRARY_RELEASE} DIRECTORY )
	elseif ( DEFINED OSG_LIBRARY_DEBUG AND EXISTS ${OSG_LIBRARY_DEBUG} )
	    get_filename_component( OSG_ROOT ${OSG_LIBRARY_DEBUG} DIRECTORY )
	elseif ( DEFINED OSG_LIBRARY )
	    list(GET OSG_LIBRARY -1 OSG_LAST_LIBRARY )
	    if ( EXISTS "${OSG_LAST_LIBRARY}" )
		get_filename_component( OSG_ROOT ${OSG_LAST_LIBRARY} DIRECTORY )
	    endif()
	    unset( OSG_LAST_LIBRARY )
	endif()
	if ( IS_DIRECTORY "${OSG_ROOT}" )
	    get_filename_component( OSG_ROOT ${OSG_ROOT} DIRECTORY )
	endif()
    endif()
    set(ENV{OSG_ROOT} "${OSG_ROOT}" )
endmacro(OD_FIND_OSGDIR)

macro( OD_CONF_OSGGEO )
    if ( NOT EXISTS "${OSGGEO_EXT_DIR}" )
	file(MAKE_DIRECTORY "${OSGGEO_EXT_DIR}")
    endif()
	set( OSG_CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )
	set( OSG_CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL} )
	set( OSG_CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} )
	set( OSG_CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} )
	if ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
	    set( OSG_CMAKE_CXX_FLAGS_DEBUG "/MDd ${OSG_CMAKE_CXX_FLAGS_DEBUG}" )
	    set( OSG_CMAKE_CXX_FLAGS_MINSIZEREL "/MD ${OSG_CMAKE_CXX_FLAGS_MINSIZEREL}" )
	    set( OSG_CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD ${OSG_CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
	    set( OSG_CMAKE_CXX_FLAGS_RELEASE "/MD ${OSG_CMAKE_CXX_FLAGS_RELEASE}" )
	endif()
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	"${EXTGENERATOR}"
	"${EXTMAKEPROG}"
	"${EXTPLFARCH}"
	"${EXTPLFTOOLSET}"
	"-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
	"-DCMAKE_CXX_FLAGS_DEBUG=${OSG_CMAKE_CXX_FLAGS_DEBUG}"
	"-DCMAKE_CXX_FLAGS_MINSIZEREL=${OSG_CMAKE_CXX_FLAGS_MINSIZEREL}"
	"-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=${OSG_CMAKE_CXX_FLAGS_RELWITHDEBINFO}"
	"-DCMAKE_CXX_FLAGS_RELEASE=${OSG_CMAKE_CXX_FLAGS_RELEASE}"
	"-DOSG_ROOT=${OSG_ROOT}"
	-DOSGGEO_LIB_POSTFIX=
	-DOSGGEO_USE_DEBUG_OSG=ON
	-DBUILD_EXAMPLES=OFF
	"-DCMAKE_INSTALL_PREFIX=${OSGGEO_EXT_DIR}/inst"
	"-DCMAKE_INSTALL_RPATH=\\$ORIGIN"
	"${CMAKE_SOURCE_DIR}/external/osgGeo"
	WORKING_DIRECTORY "${OSGGEO_EXT_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( SEND_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_CONF_OSGGEO)

macro( OD_BUILD_OSGGEO )
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	--build "${OSGGEO_EXT_DIR}"
	--config Debug
	--target install
	--clean-first
	WORKING_DIRECTORY "${OSGGEO_EXT_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( ${STATUS} EQUAL 0 )
	execute_process(
	    COMMAND "${CMAKE_COMMAND}"
	    --build "${OSGGEO_EXT_DIR}"
	    --config RelWithDebInfo
	    --target install
	    WORKING_DIRECTORY "${OSGGEO_EXT_DIR}"
	    ERROR_VARIABLE ERROUTPUT
	    RESULT_VARIABLE STATUS )
	if ( ${STATUS} EQUAL 0 )
	    set( OD_OSGGEO_ISBUILT TRUE )
	else()
	    message( SEND_ERROR "${ERROUTPUT}" )
	endif()
    else()
	message( SEND_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_BUILD_OSGGEO)

macro( OD_POSTBUILD_OSGGEO )
    if ( OD_ENABLE_BREAKPAD AND EXISTS "${BREAKPAD_DUMPSYMS_EXECUTABLE}" )
	osggeo_get_symbols()
    endif()
    if ( "${CTEST_MODEL}" STREQUAL "Experimental" OR
	 "${CTEST_MODEL}" STREQUAL "Nightly" )
	OD_STRIP_OSGGEO()
    endif()
endmacro(OD_POSTBUILD_OSGGEO)

macro( OD_STRIP_OSGGEO )
    if ( WIN32 )
	set( osg_config "install" )
    else()
	set( osg_config "install/strip" )
    endif()
    execute_process(
	COMMAND "${CMAKE_COMMAND}"
	--build "${OSGGEO_EXT_DIR}"
	--config Release
	--target "${osg_config}"
	WORKING_DIRECTORY "${OSGGEO_EXT_DIR}"
	ERROR_VARIABLE ERROUTPUT
	RESULT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL 0 )
	message( SEND_ERROR "${ERROUTPUT}" )
    endif()
endmacro(OD_STRIP_OSGGEO)

function( osggeo_get_symbols )
    cmake_policy(PUSH)
    cmake_policy(SET CMP0017 NEW)
    find_package( OpenSceneGraph QUIET COMPONENTS osgGeo )
    cmake_policy(POP)
    if( NOT OSGGEO_FOUND )
	message( FATAL_ERROR "OpenSceneGraph-osgGeo not found" )
    endif()
    OD_OSG_CREATETARGETS( osgGeo )
    get_target_property( OSGGEO_CONFIGS osg::osgGeo IMPORTED_CONFIGURATIONS )
    foreach( config ${OSGGEO_CONFIGS} )
	get_target_property( OSGGEO_LIBRARY osg::osgGeo IMPORTED_LOCATION_${config} )
	if ( EXISTS "${OSGGEO_LIBRARY}" )
	    if ( UNIX )
		get_target_property( OSGGEO_SONAME osg::osgGeo IMPORTED_SONAME_${config} )
		get_filename_component( OSGGEO_LIBRARY "${OSGGEO_LIBRARY}" DIRECTORY )
		set( OSGGEO_LIBRARY "${OSGGEO_LIBRARY}/${OSGGEO_SONAME}" )
	    endif()
	    execute_process( COMMAND ${CMAKE_COMMAND}
		    "-DLIBRARY=${OSGGEO_LIBRARY}"
		    "-DSYM_DUMP_EXECUTABLE=${BREAKPAD_DUMPSYMS_EXECUTABLE}"
		    -P ${OpendTect_DIR}/CMakeModules/GenerateSymbols.cmake
		    ERROR_VARIABLE ERROUTPUT
		    RESULT_VARIABLE STATUS )
	    if ( NOT ${STATUS} EQUAL 0 )
		message( WARNING "Error generating symbols for osgGeo: ${ERROUTPUT}" )
	    endif()
	endif()
	unset( OSGGEO_LIBRARY )
    endforeach()
    unset( OSGGEO_CONFIGS )
endfunction(osggeo_get_symbols)

macro( OD_OSGGEO_COPYSYMBOLS ODMODNM )
    if ( WIN32 )
	if ( CMAKE_VERSION VERSION_GREATER_EQUAL 3.27 AND "${CMAKE_DEBUG_POSTFIX}" STREQUAL "" )
	    set( OSGGEO_SYMBS_INPPATH "${OSGGEO_DIR}/bin/symbols/$<TARGET_IMPORT_FILE_BASE_NAME:osg::osgGeo>.pdb" )
	    set( OSGGEO_SYMBS_OUTPATH "${OD_RUNTIME_DIRECTORY}/symbols/$<TARGET_IMPORT_FILE_BASE_NAME:osg::osgGeo>.pdb" )
	else()
	    get_target_property( OSGGEO_FILENAME_DEBUG osg::osgGeo IMPORTED_LOCATION_DEBUG )
	    get_target_property( OSGGEO_FILENAME_RELEASE osg::osgGeo IMPORTED_LOCATION_RELWITHDEBINFO )
	    if ( NOT OSGGEO_FILENAME_RELEASE )
		get_target_property( OSGGEO_FILENAME_RELEASE osg::osgGeo IMPORTED_LOCATION_RELEASE )
	    endif()
	    if ( NOT OSGGEO_FILENAME_DEBUG )
		set( OSGGEO_FILENAME_DEBUG "${OSGGEO_FILENAME_RELEASE}" )
	    endif()
	    get_filename_component( OSGGEO_FILENAME_RELEASE "${OSGGEO_FILENAME_RELEASE}" NAME_WE )
	    get_filename_component( OSGGEO_FILENAME_DEBUG "${OSGGEO_FILENAME_DEBUG}" NAME_WE )
	    set( OSGGEO_SYMBS_INPPATH "${OSGGEO_DIR}/bin/symbols/$<IF:$<CONFIG:Debug>,${OSGGEO_FILENAME_DEBUG},${OSGGEO_FILENAME_RELEASE}>.pdb" )
	    set( OSGGEO_SYMBS_OUTPATH "${OD_RUNTIME_DIRECTORY}/symbols/$<IF:$<CONFIG:Debug>,${OSGGEO_FILENAME_DEBUG},${OSGGEO_FILENAME_RELEASE}>.pdb" )
	    unset( OSGGEO_FILENAME_DEBUG )
	    unset( OSGGEO_FILENAME_RELEASE )
	endif()
    else()
	if ( CMAKE_VERSION VERSION_GREATER_EQUAL 3.27 )
	    set( OSGGEO_SYMBS_INPPATH "${OSGGEO_DIR}/lib/symbols/$<TARGET_SONAME_FILE_NAME:osg::osgGeo>" )
	    set( OSGGEO_SYMBS_OUTPATH "${OD_LIBRARY_DIRECTORY}/symbols/$<TARGET_SONAME_FILE_NAME:osg::osgGeo>" )
	else()
	    get_target_property( OSGGEO_SONAME_DEBUG osg::osgGeo IMPORTED_SONAME_DEBUG )
	    get_target_property( OSGGEO_SONAME_RELEASE osg::osgGeo IMPORTED_SONAME_RELWITHDEBINFO )
	    if ( NOT OSGGEO_SONAME_RELEASE )
		get_target_property( OSGGEO_SONAME_RELEASE osg::osgGeo IMPORTED_SONAME_RELEASE )
	    endif()
	    if ( NOT OSGGEO_SONAME_DEBUG )
		set( OSGGEO_SONAME_DEBUG ${OSGGEO_SONAME_RELEASE} )
	    endif()
	    set( OSGGEO_SYMBS_INPPATH "${OSGGEO_DIR}/lib/symbols/$<IF:$<CONFIG:Debug>,${OSGGEO_SONAME_DEBUG},${OSGGEO_SONAME_RELEASE}>" )
	    set( OSGGEO_SYMBS_OUTPATH "${OD_LIBRARY_DIRECTORY}/symbols/$<IF:$<CONFIG:Debug>,${OSGGEO_SONAME_DEBUG},${OSGGEO_SONAME_RELEASE}>" )
	    unset( OSGGEO_SONAME_DEBUG )
	    unset( OSGGEO_SONAME_RELEASE )
	endif()
    endif()
    add_custom_command( TARGET ${ODMODNM} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${OSGGEO_SYMBS_INPPATH}" "${OSGGEO_SYMBS_OUTPATH}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" )
endmacro()

macro( OD_CLEANUP_OSG )
    foreach( OSGMODULE ${OSGMODULES} )
	string( TOUPPER ${OSGMODULE} OSGMOD )
	unset( ${OSGMOD}_INCLUDE_DIR CACHE )
	unset( ${OSGMOD}_LIBRARY_DEBUG CACHE )
	unset( ${OSGMOD}_LIBRARY_RELEASE CACHE )
	unset( OSGMOD )
    endforeach()
    unset( OSG_FIND_MODULES )
endmacro(OD_CLEANUP_OSG)

macro( OD_OSG_CREATETARGETS OSGMODULES )
    foreach( OSGMODULE ${OSGMODULES} )
	if ( TARGET osg::${OSGMODULE} )
	    continue()
	endif()
	string( TOUPPER ${OSGMODULE} OSGMOD )
	add_library( osg::${OSGMODULE} SHARED IMPORTED GLOBAL )
	if ( ${OSGMOD}_LIBRARY_RELEASE )
	    list( APPEND ${OSGMOD}_CONFIGS RELEASE )
	endif()
	if ( ${OSGMOD}_LIBRARY_DEBUG )
	    list( APPEND ${OSGMOD}_CONFIGS DEBUG )
	endif()
	if ( NOT ${OSGMOD}_CONFIGS )
	    message( SEND_ERROR "${OSGMOD}_LIBRARY configurations not found" )
	    continue()
	endif()
	set_target_properties( osg::${OSGMODULE} PROPERTIES
		OUTPUT_NAME "${OSGMODULE}"
		OUTPUT_NAME_DEBUG "${OSGMODULE}d"
		IMPORTED_CONFIGURATIONS "${${OSGMOD}_CONFIGS}"
		INTERFACE_INCLUDE_DIRECTORIES "${${OSGMOD}_INCLUDE_DIR}" )
	unset( ${OSGMOD}_LOCATION )
	foreach( config ${${OSGMOD}_CONFIGS} )
	    if ( EXISTS "${${OSGMOD}_LIBRARY_${config}}" )
		set( ${OSGMOD}_LOCATION_${config} "${${OSGMOD}_LIBRARY_${config}}" )
		if ( UNIX )
		    if ( IS_SYMLINK "${${OSGMOD}_LOCATION_${config}}" )
			file( READ_SYMLINK "${${OSGMOD}_LOCATION_${config}}" ${OSGMOD}_SONAME_${config} )
		    else()
			get_filename_component( ${OSGMOD}_SONAME_${config} "${${OSGMOD}_LOCATION_${config}}" NAME )
		    endif()
		endif()
		get_filename_component( ${OSGMOD}_LOCATION_${config} "${${OSGMOD}_LOCATION_${config}}" REALPATH )
		set( ${OSGMOD}_LIB_LOCATION_${config} "${${OSGMOD}_LOCATION_${config}}" )
		if ( WIN32 )
		    od_get_dll( "${${OSGMOD}_LIB_LOCATION_${config}}" ${OSGMOD}_LOCATION_${config} )
		endif()
		set_target_properties( osg::${OSGMODULE} PROPERTIES
		      IMPORTED_LOCATION_${config} "${${OSGMOD}_LOCATION_${config}}" )
		if ( WIN32 )
		    set_target_properties( osg::${OSGMODULE} PROPERTIES
			  IMPORTED_IMPLIB_${config} "${${OSGMOD}_LIB_LOCATION_${config}}" )
		else()
		    set_target_properties( osg::${OSGMODULE} PROPERTIES
			  IMPORTED_SONAME_${config} ${${OSGMOD}_SONAME_${config}} )
		endif()
	    endif()
	    unset( ${OSGMOD}_LOCATION_${config} )
	    unset( ${OSGMOD}_LIB_LOCATION_${config} )
	    unset( ${OSGMOD}_SONAME_${config} )
	endforeach()
	od_map_configurations( osg::${OSGMODULE} )
	unset( ${OSGMOD}_CONFIGS )
	unset( OSGMOD )
    endforeach()

    if ( UNIX AND osgText IN_LIST OSGMODULES )
	add_fontconfig( OSG_TEXT_IMPORTED_OBJECTS )
	set_target_properties( osg::osgText PROPERTIES
	    IMPORTED_LINK_DEPENDENT_LIBRARIES "${OSG_TEXT_IMPORTED_OBJECTS}" )
	unset( OSG_TEXT_IMPORTED_OBJECTS )
    endif()
endmacro()

macro( OD_ADD_OSGGEO )

    if ( NOT OD_NO_OSG )
	OD_FIND_OSGDIR()
	if ( IS_DIRECTORY "${OSG_ROOT}" )
	    if ( IS_DIRECTORY "${OpendTect_DIR}/external" )
		set( OSGGEO_EXT_DIR "${OD_BINARY_BASEDIR}/external/osgGeo" )
		if ( NOT EXISTS "${OSGGEO_EXT_DIR}/CMakeCache.txt" )
		    OD_CONF_OSGGEO()
		    OD_BUILD_OSGGEO()
		elseif ( NOT EXISTS "${OSGGEO_EXT_DIR}/inst" )
		    OD_BUILD_OSGGEO()
		endif()

		if ( APPLE )
		    set( OSGGEO_DIR "${OSGGEO_EXT_DIR}/inst/Contents/Frameworks" )
		    set( OSGGEO_INCLUDE_DIR "${OSGGEO_EXT_DIR}/inst/Contents/Resources/include" )
		    list( APPEND CMAKE_MODULE_PATH "${OSGGEO_EXT_DIR}/inst/Contents/Resources/share/CMakeModules" )
		else()
		    set( OSGGEO_DIR "${OSGGEO_EXT_DIR}/inst" )
		    set( OSGGEO_INCLUDE_DIR "${OSGGEO_DIR}/include" )
		    list( APPEND CMAKE_MODULE_PATH "${OSGGEO_DIR}/share/CMakeModules" )
		endif()

		if ( OD_OSGGEO_ISBUILT )
		    OD_POSTBUILD_OSGGEO()
		    unset( OD_OSGGEO_ISBUILT )
		endif()
	    endif()
	endif()
    endif()

endmacro(OD_ADD_OSGGEO)

macro( OD_FIND_OSG )

    if ( NOT OD_NO_OSG )
	OD_FIND_OSGDIR()
	cmake_policy(PUSH)
	cmake_policy(SET CMP0017 NEW)
	set( OSG_FIND_MODULES osgDB;osgGA;osgUtil;osgManipulator;osgWidget;osgViewer;osgVolume;osgText;osgSim )
	find_package( OpenSceneGraph QUIET COMPONENTS ${OSG_FIND_MODULES} GLOBAL )
	find_package( OpenSceneGraph QUIET COMPONENTS osgGeo GLOBAL )
	cmake_policy(POP)

	if ( IS_DIRECTORY "${OpendTect_DIR}/external" )
	    set( REQUIRE_OSGGEO TRUE )
	endif()

	if ( REQUIRE_OSGGEO AND OSGGEO_FOUND )
	    list( APPEND OSG_FIND_MODULES osgGeo )
	endif()

	set( OSGMODULES osg "${OSG_FIND_MODULES}" OpenThreads )
	if ( OSG_FOUND AND OPENTHREADS_FOUND )
	    OD_OSG_CREATETARGETS( "${OSGMODULES}" )
	else()
	    set( OSG_ROOT "" CACHE PATH "OSG Location" )
	    if ( NOT OSG_FOUND )
		message( SEND_ERROR "Cannot find the OpenSceneGraph installation" )
	    elseif ( NOT OPENTHREADS_FOUND )
		message( SEND_ERROR "Cannot find the OpenThreads library" )
	    elseif( REQUIRE_OSGGEO AND NOT OSGGEO_FOUND )
		message( SEND_ERROR "Cannot find the OpenSceneGraph-osgGeo library" )
	    endif()
	endif()
	OD_CLEANUP_OSG()

    endif()

endmacro(OD_FIND_OSG)

macro(OD_SETUP_OSG)

    if ( OD_NO_OSG )
	list ( APPEND OD_MODULE_COMPILE_DEFINITIONS "OD_NO_OSG" )
    else()
	if ( TARGET osg::osg AND TARGET osg::OpenThreads AND TARGET osg::osgGeo )
	    if ( OD_EXTRA_OSGFLAGS )
		list( APPEND OD_MODULE_COMPILE_OPTIONS "${OD_EXTRA_OSGFLAGS}" )
	    endif ( OD_EXTRA_OSGFLAGS )

	    foreach( OSGMODULE ${OSGMODULES} )
		list ( APPEND OD_MODULE_EXTERNAL_LIBS osg::${OSGMODULE} )
	    endforeach()
	else()
	    if ( NOT TARGET osg::osg )
		message( SEND_ERROR "Cannot use the OpenSceneGraph installation" )
	    elseif( NOT TARGET osg::OpenThreads )
		message( SEND_ERROR "Cannot use the OpenSceneGraph-OpenThreads library" )
	    elseif( NOT TARGET osg::osgGeo )
		message( SEND_ERROR "Cannot use the OpenSceneGraph-osgGeo library" )
	    endif()
	endif()
    endif()

endmacro(OD_SETUP_OSG)

function(OD_MACOS_ADD_EXTERNAL_LIBS _libs_var)
    set(_updated_libs ${${_libs_var}})
    
    if(APPLE AND "osg::osgText" IN_LIST _updated_libs)
        if(TARGET Fontconfig::Fontconfig)
            list(APPEND _updated_libs Fontconfig::Fontconfig)
        endif()
        if(TARGET Freetype::Freetype)
            list(APPEND _updated_libs Freetype::Freetype)
        endif()
        if(TARGET PNG::PNG)
            list(APPEND _updated_libs PNG::PNG)
        endif()
    endif()
    
    set(${_libs_var} ${_updated_libs} PARENT_SCOPE)
endfunction()
