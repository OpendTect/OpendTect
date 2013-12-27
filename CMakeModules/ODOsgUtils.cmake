#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________


set(OSG_DIR "" CACHE PATH "OSG Location" )

if ( (EXISTS ${CMAKE_SOURCE_DIR}/external/osgGeo/CMakeLists.txt) AND
     (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR) AND
    ((NOT DEFINED osgGeo_DIR) OR
	(osgGeo_DIR STREQUAL "") OR
	(osgGeo_DIR MATCHES "-NOTFOUND")) AND
    (DEFINED OSG_DIR) AND
    (NOT OSG_DIR STREQUAL "") AND
    (NOT OSG_DIR MATCHES "-NOTFOUND") )
    set ( osgGeo_DIR ${CMAKE_BINARY_DIR}/external/osgGeo CACHE PATH
	  "osgGeo location" FORCE )

    if ( (CMAKE_GENERATOR STREQUAL "Unix Makefiles") OR
	 (CMAKE_GENERATOR STREQUAL "Ninja") )
	if ( NOT EXISTS ${CMAKE_BINARY_DIR}/external/osgGeo )
	    execute_process ( COMMAND ${CMAKE_COMMAND} -E create_directory
			      "${osgGeo_DIR}" )

	endif()
	execute_process ( COMMAND ${CMAKE_COMMAND} . -DOSG_DIR=${OSG_DIR}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/external/osgGeo
		-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
		-G${CMAKE_GENERATOR}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/osgGeo/ )

	execute_process ( COMMAND ${CMAKE_COMMAND}
		    --build ${CMAKE_SOURCE_DIR}/external/osgGeo
		    --target osgGeo )
    else()
	execute_process ( COMMAND ${CMAKE_COMMAND} . -DOSG_DIR=${OSG_DIR}
		-G${CMAKE_GENERATOR}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/osgGeo/ )
	execute_process ( COMMAND ${CMAKE_COMMAND}
		--build ${CMAKE_SOURCE_DIR}/external/osgGeo
		--target osgGeo
		--config Debug )
	execute_process ( COMMAND ${CMAKE_COMMAND}
		--build ${CMAKE_SOURCE_DIR}/external/osgGeo
		--target osgGeo
		--config Release )
    endif()
endif()

if ( osgGeo_DIR MATCHES "${CMAKE_SOURCE_DIR}/external/osgGeo" )
    if ( (CMAKE_GENERATOR STREQUAL "Unix Makefiles") OR
	 (CMAKE_GENERATOR STREQUAL "Ninja") )
	add_custom_target( osgGeo ALL COMMAND ${CMAKE_COMMAND}
		    --build ${CMAKE_SOURCE_DIR}/external/osgGeo
		    --target osgGeo )
    else()
	add_custom_target( osgGeo ALL
		${CMAKE_COMMAND}
		    --build ${CMAKE_SOURCE_DIR}/external/osgGeo
		    --target osgGeo
		    --config Debug
		COMMAND ${CMAKE_COMMAND}
		    --build ${CMAKE_SOURCE_DIR}/external/osgGeo
		    --config Release
		    --target osgGeo )
    endif()
endif()

macro(OD_SETUP_OSG)

    if ( (NOT DEFINED osgGeo_DIR) OR
	 (osgGeo_DIR STREQUAL "") OR
	 (osgGeo_DIR MATCHES "-NOTFOUND"))
        set(osgGeo_DIR ${OSG_DIR})
    endif()

    list(APPEND CMAKE_MODULE_PATH ${osgGeo_DIR}/share/CMakeModules )
    list(APPEND CMAKE_MODULE_PATH ${osgGeo_DIR}/CMakeModules )
    list(APPEND CMAKE_MODULE_PATH ${OSG_DIR}/share/CMakeModules )

    #SET DEBUG POSTFIX
    set (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    set (CMAKE_DEBUG_POSTFIX d)

    find_package( OpenGL )
    find_package(OSG)
    find_package(osgGeo)

    #RESTORE DEBUG POSTFIX
    set (CMAKE_DEBUG_POSTFIX ${OLD_CMAKE_DEBUG_POSTFIX} )

    if ( (NOT DEFINED OSG_FOUND) OR (NOT DEFINED OSGGEO_FOUND) )
	MESSAGE( FATAL_ERROR "OSG_DIR and/or osgGeo not set")
    endif()


    if(OD_USEOSG)
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
		OSGQT
		OSGMANIPULATOR
		OSGWIDGET
		OSGVIEWER
		OSGVOLUME
		OPENTHREADS
		OSGTEXT
		OSGSIM
		OSGGEO )

	foreach( OSGMODULE ${OSGMODULES} )
	    if ( ${OSGMODULE}_LIBRARY )
		list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY} )
	    else()
		message(FATAL_ERROR "${OSGMODULE}_LIBRARY is missing")
	    endif()
	endforeach()

	if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	    foreach ( BUILD_TYPE Debug Release )

		set( OARGS  ${OD_OSG_LIBS} )
		unset ( ARGS )

		OD_FILTER_LIBRARIES( OARGS ${BUILD_TYPE} )

		foreach( LIB ${OARGS} )
		    get_filename_component( OSGLIBNAME ${LIB} NAME_WE )
		    if( UNIX OR APPLE )
			if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
			    #installing linx osgfiles(like libosgQt.so.80,libosgDB.so.80 etc. )
			    file( GLOB ALLLIBS "${OSG_DIR}/lib/${OSGLIBNAME}.so.[0-9][0-9]" )
			elseif( APPLE )
			    #installing Mac osgfiles(like libosgQt.80.dylib,libosgDB.80.dylib etc. )
			    file( GLOB ALLLIBS "${OSG_DIR}/lib/${OSGLIBNAME}.[0-9][0-9].dylib" )
			endif()
			list( APPEND ARGS ${ALLLIBS} )
			list( LENGTH ARGS LEN )
			if( "${LEN}" EQUAL "0" )
			    get_filename_component( OSGLIBNAME ${LIB} NAME )
			    list( APPEND ARGS ${OSG_DIR}/lib/${OSGLIBNAME} )
			endif()
			list( GET ARGS 0 FILENM )
			OD_INSTALL_LIBRARY( ${FILENM} ${BUILD_TYPE} )
			list( REMOVE_ITEM ARGS ${ARGS} )
			set( ALLLIBS "" )
		    elseif( WIN32 )
			file ( GLOB DLLFILE "${OSG_DIR}/bin/*${OSGLIBNAME}.dll" )
			if ( EXISTS ${DLLFILE} )
			    OD_INSTALL_LIBRARY( ${DLLFILE} ${BUILD_TYPE} )
			    install( PROGRAMS ${LIB}
				DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${BUILD_TYPE}
				CONFIGURATIONS ${BUILD_TYPE} )
			endif()
		    endif()
		endforeach()

	    endforeach()
	endif()
    endif()

    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_OSG_LIBS} )

endmacro(OD_SETUP_OSG)


