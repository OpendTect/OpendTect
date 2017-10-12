#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set( QTDIR "" CACHE PATH "QT Location" )
option ( OD_NO_QT "Turn off all QT" NO )

##Create launcher for linguist
set( LINGUIST_LAUNCHER "CMakeModules/templates/linguist.csh.in" )
if ( EXISTS ${LINGUIST_LAUNCHER} )
    configure_file( ${LINGUIST_LAUNCHER} dtect/linguist.csh @ONLY )
endif()

macro(ADD_TO_LIST_IF_NEW LISTNAME ITEMNAME)
    list( FIND ${LISTNAME} "${ITEMNAME}" ITMINDEX )
    if ( ${ITMINDEX} EQUAL -1 )
	list(APPEND ${LISTNAME} "${ITEMNAME}")
    endif()
endmacro(ADD_TO_LIST_IF_NEW)

macro(OD_SETUP_QT)
    if ( OD_NO_QT )
	add_definitions( -DOD_NO_QT )
    else()
	if ( (NOT DEFINED QTDIR) OR QTDIR STREQUAL "" )
	    MESSAGE( FATAL_ERROR "QTDIR not set")
	endif()

	#Try to find Qt5
	list ( APPEND CMAKE_PREFIX_PATH ${QTDIR} )
	find_package( Qt5 REQUIRED ${OD_USEQT} )

	    if( QT_MOC_HEADERS )
		foreach( HEADER ${QT_MOC_HEADERS} )
		    list( APPEND QT_MOC_INPUT
			  ${CMAKE_SOURCE_DIR}/src/${OD_MODULE_NAME}/${HEADER} )
		endforeach()
		QT5_WRAP_CPP( QT_MOC_OUTFILES ${QT_MOC_INPUT} )
	    endif( QT_MOC_HEADERS )

	    foreach ( QTMOD ${OD_USEQT} )
		list( APPEND OD_MODULE_INCLUDESYSPATH ${Qt5${QTMOD}_INCLUDE_DIRS} )
		list( APPEND OD_QT_LIBS ${Qt5${QTMOD}_LIBRARIES} )

		if ( NOT DEFINED QT_LIBRARY_DIR )
		    get_target_property(QT_LIBRARY Qt5::${QTMOD} LOCATION )
		    get_filename_component( QT_LIBRARY_DIR ${QT_LIBRARY} PATH )
		endif()
	    endforeach()

	    list ( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${QT_LIBRARY_DIR} )

	    list( REMOVE_DUPLICATES OD_QT_LIBS )
	    list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )

	    if ( WIN32 )
		set ( CMAKE_CXX_FLAGS "/wd4481 ${CMAKE_CXX_FLAGS}" )
	    endif( WIN32 )

    endif( OD_NO_QT )
endmacro( OD_SETUP_QT )

macro ( SETUP_QT_TRANSLATION POSTFIX )
endmacro( SETUP_QT_TRANSLATION )
