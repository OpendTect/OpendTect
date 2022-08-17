#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
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
	set ( INITHEADER_DIR ${PROJECT_BINARY_DIR}/plugins/${OD_MODULE_NAME} )
	set ( EXPORTHEADER_DIR ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME} )
    else ()
	if ( EXISTS ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	    set ( INITHEADER_DIR ${PROJECT_BINARY_DIR}/include/${OD_MODULE_NAME} )
	    set ( EXPORTHEADER_DIR ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	else()
	    set ( INITHEADER_DIR ${PROJECT_BINARY_DIR}/spec/${OD_MODULE_NAME} )
	    set ( EXPORTHEADER_DIR ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
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

	configure_file( ${OpendTect_DIR}/CMakeModules/templates/moddeps.h.in 
		    ${DEPSHEADER} )
    endif()

    configure_file( ${OpendTect_DIR}/CMakeModules/templates/moduleheader.h.in 
		    ${INITHEADER} )
endmacro()
