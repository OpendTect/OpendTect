#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

macro( OD_ADD_OSG )
    if ( NOT DEFINED OSG_DIR )
	set( OSG_DIR "" CACHE PATH "OSG Location" )
	if ( DEFINED OSG_LIBRARY )
	    get_filename_component( OSG_DIR ${OSG_LIBRARY} DIRECTORY )
	    get_filename_component( OSG_DIR ${OSG_DIR} DIRECTORY )
	elseif ( DEFINED OSG_LIBRARY_RELEASE )
	    get_filename_component( OSG_DIR ${OSG_LIBRARY_RELEASE} DIRECTORY )
	    get_filename_component( OSG_DIR ${OSG_DIR} DIRECTORY )
	elseif ( DEFINED OSG_LIBRARY_DEBUG )
	    get_filename_component( OSG_DIR ${OSG_LIBRARY_DEBUG} DIRECTORY )
	    get_filename_component( OSG_DIR ${OSG_DIR} DIRECTORY )
	else()
	    message( FATAL_ERROR "OSG_DIR is not defined" )
	endif()
    endif()

    set(ENV{OSG_DIR} ${OSG_DIR})

    #SET DEBUG POSTFIX
    set (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    set (CMAKE_DEBUG_POSTFIX d)

    find_package( OpenSceneGraph REQUIRED osgDB osgGA osgUtil osgManipulator osgWidget osgViewer osgVolume osgText osgSim )

    #RESTORE DEBUG POSTFIX
    set (CMAKE_DEBUG_POSTFIX ${OLD_CMAKE_DEBUG_POSTFIX} )
    if ( (NOT DEFINED OSG_FOUND) )
       message( FATAL_ERROR "Cannot find/use the OSG installation" )
    endif()
    unset( OSG_DIR CACHE )

endmacro()

macro( OD_ADD_OSGGEO )

    if ( (NOT DEFINED OSG_FOUND) )
	OD_ADD_OSG()
	if ( (NOT DEFINED OSG_FOUND) )
	    MESSAGE( FATAL_ERROR "OSG_DIR not set" )
	endif()
    endif()

    list(APPEND CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/external/osgGeo/CMakeModules )

    #SET DEBUG POSTFIX
    set (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    set (CMAKE_DEBUG_POSTFIX d)

    set ( OSGGEO_INSTDIR_LIB_DEBUG ${OD_LIB_INSTALL_PATH_DEBUG} )
    set ( OSGGEO_INSTDIR_LIB_RELEASE ${OD_LIB_INSTALL_PATH_RELEASE} )

    add_subdirectory( ${CMAKE_SOURCE_DIR}/external/osgGeo/src/osgGeo 
		  ${CMAKE_BINARY_DIR}/external/osgGeo/src/osgGeo )

    #RESTORE DEBUG POSTFIX

    set (CMAKE_DEBUG_POSTFIX ${OLD_CMAKE_DEBUG_POSTFIX} )

    set ( OSGGEO_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/osgGeo/src
			     ${CMAKE_BINARY_DIR}/external/osgGeo/src )
    set ( OSGGEO_LIBRARY_PATH
	${CMAKE_BINARY_DIR}/external/osgGeo/src/osgGeo/${CMAKE_BUILD_TYPE} )
endmacro()

macro(OD_SETUP_OSG)

    if ( (NOT DEFINED OSG_FOUND) )
	OD_ADD_OSG()
    endif()

    if(OD_USEOSG)
	list(APPEND OD_MODULE_INCLUDESYSPATH
		${OSGGEO_INCLUDE_DIR}
		${OSG_INCLUDE_DIR} )
	list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )

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
		OSGSIM )

	foreach( OSGMODULE ${OSGMODULES} )
	    if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
		if ( ${OSGMODULE}_LIBRARY )
		    list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY} )
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

	list ( APPEND OD_OSG_LIBS osgGeo )
	list ( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${OSGGEO_LIBRARY_PATH} )

	if ( OD_INSTALL_DEPENDENT_LIBS )
	    foreach ( BUILD_TYPE Debug Release )

		set( OARGS  ${OD_OSG_LIBS} )
		unset ( ARGS )

		OD_FILTER_LIBRARIES( OARGS ${BUILD_TYPE} )

		foreach( LIB ${OARGS} )
		    get_filename_component( OSGLIBNAME ${LIB} NAME_WE )
		    get_filename_component( OSGLIBLOC ${LIB} DIRECTORY )
		    string( FIND ${OSGLIBNAME} "osgGeo" ISOSGGEO )
		    if( UNIX OR APPLE )
			if( NOT ${ISOSGGEO} EQUAL -1 )
			    set( ALLLIBS ${LIB} )
			elseif( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
			    #installing linux osgfiles(like libosgDB.so.100 etc. )
			    file( GLOB ALLLIBS "${OSGLIBLOC}/${OSGLIBNAME}.so.[0-9][0-9]*" )
			elseif( APPLE )
			    #installing Mac osgfiles(like libosgDB.80.dylib etc. )
			    file( GLOB ALLLIBS "${OSGLIBLOC}/${OSGLIBNAME}.[0-9][0-9]*.dylib" )
			endif()
			list( APPEND ARGS ${ALLLIBS} )
			list( LENGTH ARGS LEN )
			if( "${LEN}" EQUAL "0" )
			    get_filename_component( OSGLIBNAME ${LIB} NAME )
			    list( APPEND ARGS ${OSGLIBLOC}/${OSGLIBNAME} )
			endif()
			list( GET ARGS 0 FILENM )
			if( ${ISOSGGEO} EQUAL -1 )
			    OD_INSTALL_LIBRARY( ${FILENM} ${BUILD_TYPE} )

			    if ( OD_ENABLE_BREAKPAD AND CMAKE_BUILD_TYPE )
				if ( ${CMAKE_BUILD_TYPE} STREQUAL "Release" )
				    execute_process( COMMAND ${CMAKE_COMMAND}
					-DLIBRARY=${FILENM}
					-DSYM_DUMP_EXECUTABLE=${BREAKPAD_DUMPSYMS_EXECUTABLE}
					-P ${OpendTect_DIR}/CMakeModules/GenerateSymbols.cmake )
				    get_filename_component( FILEPATH ${FILENM} PATH )
				    install( DIRECTORY ${FILEPATH}/symbols DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE} )
				endif()
			    endif()
			endif()
			list( REMOVE_ITEM ARGS ${ARGS} )
			set( ALLLIBS "" )
		    elseif( WIN32 AND (${ISOSGGEO} EQUAL -1) )
			get_filename_component( OSG_BASEPATH ${LIB} DIRECTORY )
		        get_filename_component( OSG_BASEPATH ${OSG_BASEPATH} DIRECTORY )
		        file ( GLOB DLLFILE "${OSG_BASEPATH}/bin/o*${OSGLIBNAME}.dll" )
		        file ( GLOB PDBFILE "${OSG_BASEPATH}/bin/o*${OSGLIBNAME}.pdb" )
			if ( EXISTS ${DLLFILE} )
			    OD_INSTALL_LIBRARY( ${DLLFILE} ${BUILD_TYPE} )
			    if ( OD_ENABLE_BREAKPAD AND ${CMAKE_BUILD_TYPE} STREQUAL "Release" )
			        execute_process( COMMAND ${CMAKE_COMMAND}
							-DLIBRARY=${DLLFILE}
							-DSYM_DUMP_EXECUTABLE=${BREAKPAD_DUMPSYMS_EXECUTABLE}
							-P ${OpendTect_DIR}/CMakeModules/GenerateSymbols.cmake )
			        get_filename_component( DLLFILEPATH ${DLLFILE} PATH )
			        install( DIRECTORY ${DLLFILEPATH}/symbols DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE} )
			    endif()
			endif()
			if ( EXISTS ${PDBFILE} )
			    OD_INSTALL_LIBRARY( ${PDBFILE} ${BUILD_TYPE} )
			endif()
		    endif()
		endforeach()

	    endforeach()
	endif( OD_INSTALL_DEPENDENT_LIBS )
    endif()

    list( APPEND OD_MODULE_EXTERNAL_LIBS ${OD_OSG_LIBS} )

endmacro(OD_SETUP_OSG)
