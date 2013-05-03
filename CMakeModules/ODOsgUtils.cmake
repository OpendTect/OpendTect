#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________


set(OSG_DIR "" CACHE PATH "OSG Location" )

macro(OD_SETUP_OSG)
    set(OSGGEO_DIR ${OSG_DIR})

    list(APPEND CMAKE_MODULE_PATH ${OSGGEO_DIR}/share/CMakeModules )

    #SET DEBUG POSTFIX
    set (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    set (CMAKE_DEBUG_POSTFIX d)

    find_package(OSG)
    find_package(osgGeo)

    #RESTORE DEBUG POSTFIX
    set (CMAKE_DEBUG_POSTFIX ${OLD_CMAKE_DEBUG_POSTFIX} )

    if ( (NOT DEFINED OSG_FOUND) OR (NOT DEFINED OSGGEO_FOUND) )
	set(OSG_DIR "" CACHE PATH "OSG location" FORCE )
	MESSAGE( FATAL_ERROR "OSG_DIR not set")
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
		OSGGEO )

	foreach( OSGMODULE ${OSGMODULES} )
	    if (  ${OSGMODULE}_LIBRARY_DEBUG AND  ${OSGMODULE}_LIBRARY )
		list( APPEND OD_OSG_LIBS "optimized" ${${OSGMODULE}_LIBRARY}
		      "debug" ${${OSGMODULE}_LIBRARY_DEBUG} )
	    elseif ( ${OSGMODULE}_LIBRARY_DEBUG )
		list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY_DEBUG} )
	    elseif ( ${OSGMODULE}_LIBRARY )
		list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY} )
	    else()
		message(FATAL_ERROR "${OSGMODULE}_LIBRARY is missing")
	    endif()
	endforeach()

	if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	    foreach ( BUILD_TYPE "Debug" "Release" )

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
			OD_INSTALL_LIBRARY( ${FILENM} )
			list( REMOVE_ITEM ARGS ${ARGS} )
			set( ALLLIBS "" )
		    elseif( WIN32 )
			list( APPEND ARGS ${OSG_DIR}/lib/${OSGLIBNAME}.dll )
		    endif()
		endforeach()

		if ( WIN32 )
		    install( PROGRAMS ${ARGS}
			 DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${BUILD_TYPE} )
		endif()
	    endforeach()
	endif()
    endif()

    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_OSG_LIBS} )

endmacro(OD_SETUP_OSG)


