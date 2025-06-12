#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

# OD_CREATE_INIT_HEADER
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
#

macro( OD_CREATE_INIT_HEADER )
    
    string ( TOUPPER ${OD_MODULE_NAME} OD_MODULE_NAME_UPPER )
    string ( TOLOWER ${OD_MODULE_NAME} OD_MODULE_NAME_LOWER )

    if ( OD_IS_PLUGIN )
	set ( INITHEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}" )
	set ( EXPORTHEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
    else ()
	if ( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME}" )
	    set ( INITHEADER_DIR "${PROJECT_BINARY_DIR}/${MISC_INSTALL_PREFIX}/include/${OD_MODULE_NAME}" )
	    set ( EXPORTHEADER_DIR "${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME}" )
	else()
	    set ( INITHEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}" )
	    set ( EXPORTHEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
	endif()
    endif ()

    set ( INITHEADER ${INITHEADER_DIR}/${OD_MODULE_NAME_LOWER}mod.h )

    if ( WIN32 )
	set ( DEPSHEADER ${INITHEADER_DIR}/${OD_MODULE_NAME_LOWER}deps.h )
	set ( EXPORTHEADER ${OD_MODULE_NAME_LOWER}export.h )
	if ( EXISTS ${EXPORTHEADER_DIR}/${EXPORTHEADER} )
	    set ( EXPORTFILEHEADER "#include \"${EXPORTHEADER}\"" )

	    set ( INSTANTIATESOURCE "instantiate${OD_MODULE_NAME_LOWER}export.cc" )
	    configure_file( ${OpendTect_DIR}/CMakeModules/templates/instantiateexport.cc.in 
			"${CMAKE_CURRENT_BINARY_DIR}/${INSTANTIATESOURCE}" )
	    list ( APPEND OD_MODULE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${INSTANTIATESOURCE} )
	endif()

	foreach ( DEP ${OD_MODULE_DEPS} )
	    string ( TOLOWER ${DEP} DEPLOWER )
	    set ( MODFILEHEADER "${MODFILEHEADER}${OD_LINESEP}#include \"${DEPLOWER}deps.h\"" )
	endforeach()

	get_template_filepath( "moddeps.h" MODDEPSHEADER_PATH )
	configure_file( "${MODDEPSHEADER_PATH}" "${DEPSHEADER}" )
    endif()

    get_template_filepath( "moduleheader.h" MODULEHEADER_PATH )
    configure_file( "${MODULEHEADER_PATH}" "${INITHEADER}" )
endmacro()
