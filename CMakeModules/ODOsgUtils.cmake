#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODOsgUtils.cmake,v 1.14 2012-09-06 09:09:57 cvskris Exp $
#_______________________________________________________________________________


set(OSG_DIR "" CACHE PATH "OSG Location" )

MACRO(OD_SETUP_OSG)
    set(OSGGEO_DIR ${OSG_DIR})

    list(APPEND CMAKE_MODULE_PATH ${OSGGEO_DIR}/share/CMakeModules )

    #SET DEBUG POSTFIX
    set (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    set (CMAKE_DEBUG_POSTFIX d)

    FIND_PACKAGE(OSG)
    FIND_PACKAGE(osgGeo)

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
	    if ( ${OSGMODULE}_LIBRARY_DEBUG )
		list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY_DEBUG} )
	    else()
		if ( NOT ${OSGMODULE}_LIBRARY )
		    MESSAGE(FATAL_ERROR "${OSGMODULE}_LIBRARY is missing")
		endif()
		list(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY} )
	    endif()

	    if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
		install ( FILES ${${OSGMODULE}_LIBRARY} DESTINATION
		      ${OD_EXEC_INSTALL_PATH} )
	    endif()
	endforeach()
    endif()

    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_OSG_LIBS} )

ENDMACRO(OD_SETUP_OSG)
