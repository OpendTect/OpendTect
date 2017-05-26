#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	May 2017	R. Singh
#_______________________________________________________________________________

macro( OD_ADD_PROJ4 )

    #SET DEBUG POSTFIX
    set (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    set ( OLD_COMPILER_FLAGS ${CMAKE_C_FLAGS} )
    set (CMAKE_DEBUG_POSTFIX d)

    set ( PROJ4_INSTDIR_LIB_DEBUG ${OD_LIB_INSTALL_PATH_DEBUG} )
    set ( PROJ4_INSTDIR_LIB_RELEASE ${OD_LIB_INSTALL_PATH_RELEASE} )

    if ( UNIX )
	SET( CMAKE_C_FLAGS "-w " )
    elseif( WIN32 )
	SET( CMAKE_C_FLAGS "/W0 /WX- " )
    endif()

    add_subdirectory( ${CMAKE_SOURCE_DIR}/external/proj4 
		  ${CMAKE_BINARY_DIR}/external/proj4 )

    #RESTORE DEBUG POSTFIX

    set (CMAKE_DEBUG_POSTFIX ${OLD_CMAKE_DEBUG_POSTFIX} )
    set (CMAKE_C_FLAGS ${OLD_COMPILER_FLAGS} )

    set ( PROJ4_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/proj4/src
			     ${CMAKE_BINARY_DIR}/external/proj4/src )
    set ( PROJ4_LIBRARY_PATH
	${CMAKE_BINARY_DIR}/external/proj4/${CMAKE_BUILD_TYPE} )
endmacro()

macro(OD_SETUP_PROJ4)

    if ( OD_USEPROJ4 )

	if ( NOT DEFINED PROJ4_INCLUDE_DIR )
	    OD_ADD_PROJ4()
	endif()

	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${PROJ4_INCLUDE_DIR} )

	list ( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${PROJ4_LIBRARY_PATH} )

	if ( OD_INSTALL_DEPENDENT_LIBS )
	    foreach ( BUILD_TYPE Debug Release )

		set( LIB proj )

		OD_FILTER_LIBRARIES( ${LIB} ${BUILD_TYPE} )

		get_filename_component( PROJ4LIBNAME ${LIB} NAME_WE )
		    if( UNIX OR APPLE )
		        if( APPLE )
			    #installing Mac proj4 files(like libproj.12.dylib etc. )
			    file( GLOB ALLLIBS "${PROJ4_LIBRARY_PATH}/${LIB}.[0-9][0-9]*.dylib" )
	
			else()
			    #installing linux proj4 files(like libproj.so.12 etc. )
			    file( GLOB ALLLIBS "${PROJ4_LIBRARY_PATH}/${LIB}.so.[0-9][0-9]*" )
			endif()
			list( APPEND ARGS ${ALLLIBS} )
			list( GET ARGS 0 PROJ4LIB )
			OD_INSTALL_LIBRARY( ${PROJ4LIB} ${BUILD_TYPE} )

			set( PROJ4LIB "" )
		    endif()

	    endforeach()
	endif( OD_INSTALL_DEPENDENT_LIBS )
    endif()

    list( APPEND OD_MODULE_EXTERNAL_LIBS proj )

endmacro(OD_SETUP_PROJ4)
