#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

# OD_CREATE_INIT_HEADER
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
#

macro( OD_CREATE_INIT_HEADER )
    
    string ( TOUPPER ${OD_MODULE_NAME} OD_MODULE_NAME_UPPER )
    string ( TOLOWER ${OD_MODULE_NAME} OD_MODULE_NAME_LOWER )

    if ( OD_IS_PLUGIN )
	set ( INCLUDEDIR ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME} )
    else ()
	if ( EXISTS ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	    set ( INCLUDEDIR ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	else()
	    if ( EXISTS ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
		set ( INCLUDEDIR ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	    endif()
	endif()
    endif ()

    if ( EXISTS ${INCLUDEDIR} )
	set ( INITHEADER ${INCLUDEDIR}/${OD_MODULE_NAME_LOWER}mod.h )


	if ( WIN32 )
	    set ( DEPSHEADER ${INCLUDEDIR}/${OD_MODULE_NAME_LOWER}deps.h )
	    set ( EXPORTHEADER ${OD_MODULE_NAME_LOWER}export.h )
	    if ( EXISTS ${INCLUDEDIR}/${EXPORTHEADER} )
		set ( EXPORTFILEHEADER "#include \"${EXPORTHEADER}\"" )

		set ( INSTANTIATESOURCE "instantiate${OD_MODULE_NAME_LOWER}export.cc" )
		configure_file( ${OpendTect_DIR}/CMakeModules/templates/instantiateexport.cc.in 
			    "${CMAKE_CURRENT_SOURCE_DIR}/${INSTANTIATESOURCE}" )
		list ( APPEND OD_MODULE_SOURCES ${INSTANTIATESOURCE} )
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
    endif ()
endmacro()
