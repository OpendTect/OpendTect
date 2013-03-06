#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set( QTDIR "" CACHE PATH "QT Location" )

macro(OD_SETUP_QT)
    if ( (NOT DEFINED QTDIR) OR QTDIR STREQUAL "" )
	MESSAGE( FATAL_ERROR "QTDIR not set")
    endif()

    #Try to find Qt5
    list ( APPEND CMAKE_PREFIX_PATH ${QTDIR} )
    find_package( Qt5Widgets QUIET )
    if ( Qt5Widgets_FOUND )
	cmake_minimum_required( VERSION 2.8.9 )
	set( CMAKE_AUTOMOC ON )
    else()
	set( ENV{QTDIR} ${QTDIR} )
	set ( QT_QMAKE_EXECUTABLE ${QTDIR}/bin/qmake${CMAKE_EXECUTABLE_SUFFIX} )
	find_package(Qt4 REQUIRED QtGui QtCore QtSql QtNetwork )

	include(${QT_USE_FILE})

	if( ${OD_USEQT} MATCHES "Core" )
	list( APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTNETWORK_INCLUDE_DIR}
	    ${QT_QTCORE_INCLUDE_DIR} ${QTDIR}/include )
	set(OD_QT_LIBS ${QT_QTCORE_LIBRARY_RELEASE}
		       ${QT_QTNETWORK_LIBRARY_RELEASE})
	endif()

	if(${OD_USEQT} MATCHES "Sql" )
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTSQL_INCLUDE_DIR}
	    ${QTDIR}/include )
	set(OD_QT_LIBS ${QT_QTSQL_LIBRARY_RELEASE})
	endif()

	if(${OD_USEQT} MATCHES "Widgets")
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTCORE_INCLUDE_DIR}
	    ${QT_QTGUI_INCLUDE_DIR} ${QTDIR}/include )
	set(OD_QT_LIBS ${QT_QTGUI_LIBRARY_RELEASE})
	endif()

	if(${OD_USEQT} MATCHES "OpenGL")
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTOPENGL_INCLUDE_DIR} ${QTDIR}/include )
	set(OD_QT_LIBS ${QT_QTOPENGL_LIBRARY_RELEASE})
	endif()

	if( QT_MOC_HEADERS )
	foreach( HEADER ${QT_MOC_HEADERS} )
	    list(APPEND QT_MOC_INPUT
		${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME}/${HEADER})
	endforeach()

	QT4_WRAP_CPP (QT_MOC_OUTFILES ${QT_MOC_INPUT})
	endif( QT_MOC_HEADERS )

	list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )
	if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	    set( QARGS ${QT_QTOPENGL_LIBRARY_RELEASE} ${QT_QTCORE_LIBRARY_RELEASE}
		       ${QT_QTNETWORK_LIBRARY_RELEASE} ${QT_QTSQL_LIBRARY_RELEASE}
		       ${QT_QTGUI_LIBRARY_RELEASE} ${QT_QTXML_LIBRARY_RELEASE} )
	    foreach( QLIB ${QARGS} )
		get_filename_component( QLIBNAME ${QLIB} NAME_WE )
		if( UNIX OR APPLE )
		    if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
			set( FILENM "${QLIBNAME}.so.${QT_VERSION_MAJOR}" )
		    elseif( APPLE )
			set( FILENM "${QLIBNAME}.${QT_VERSION_MAJOR}.dylib" )
		    endif()
		    OD_INSTALL_LIBRARY( ${QTDIR}/lib/${FILENM} )
		elseif( WIN32 )
		    list( APPEND ARGS ${QTDIR}/bin/${QLIBNAME}.dll )
		endif()
	    endforeach()
	    if( WIN32 )
		install( PROGRAMS ${ARGS}
			 DESTINATION ${CMAKE_INSATLL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
	    endif()
	endif()
    endif()
endmacro( OD_SETUP_QT )
