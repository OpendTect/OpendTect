#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODMacroUtils.cmake,v 1.1 2012-01-11 11:43:19 cvskris Exp $
#_______________________________________________________________________________

MACRO( OD_INIT_MODULE )

#Add all module dependencies
IF(DEPS)
    OD_ADD_DEPS( ${DEPS} )
ENDIF()

#Add Qt-stuff
IF(OD_USEQT)
    include(${QT_USE_FILE})

    IF(${OD_USEQT} MATCHES "Core" )
	LIST(APPEND ${${LIB_NAME}_INCLUDEPATH}
	    ${QT_QTCORE_INCLUDE_DIR} ${QTDIR}/include )
	SET(OWN_QTLIBS ${QT_QTCORE_LIBRARY})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Sql" )
	LIST(APPEND ${${LIB_NAME}_INCLUDEPATH}
	    ${QT_QTSQL_INCLUDE_DIR} ${QTDIR}/include )
	SET(OWN_QTLIBS ${QT_QTSQL_LIBRARY})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Gui")
	LIST(APPEND ${${LIB_NAME}_INCLUDEPATH}
	    ${QT_QTCORE_INCLUDE_DIR}
	    ${QT_QTGUI_INCLUDE_DIR} ${QTDIR}/include )
	SET(OWN_QTLIBS ${QT_QTGUI_LIBRARY})
    ENDIF()

    IF( MOC_HEADERS )
	FOREACH( HEADER ${MOC_HEADERS} )
	    LIST(APPEND MOC_INPUT
		${OpendTect_SOURCE_DIR}/include/${LIB_NAME}/${HEADER})
	ENDFOREACH()

	QT4_WRAP_CPP (MOC_OUTFILES ${MOC_INPUT})
    ENDIF( MOC_HEADERS )
ENDIF(OD_USEQT)

#Add current module to include-path
LIST(APPEND ${LIB_NAME}_INCLUDEPATH ${OpendTect_SOURCE_DIR}/include/${LIB_NAME} )

#Set current include_path
INCLUDE_DIRECTORIES( ${${LIB_NAME}_INCLUDEPATH} )

#Export dependencies
SET( ${LIB_NAME}_DEPS ${${LIB_NAME}_DEPS} PARENT_SCOPE )

#Setup libraries & its deps
ADD_LIBRARY( ${LIB_NAME} SHARED ${LIB_SOURCES} ${MOC_OUTFILES} )
target_link_libraries(
        ${LIB_NAME}
        ${${LIB_NAME}_DEPS}
        ${EXTRA_LIBS} 
	${OWN_QTLIBS} )

#Add executable targets
IF(EXEC_SOURCES)
    FOREACH( EXEC ${EXEC_SOURCES} )
	ADD_EXECUTABLE( ${EXEC} ${EXEC}.cc )
	target_link_libraries(
	    ${EXEC}
	    ${LIB_NAME}
	    ${${LIB_NAME}_DEPS}
	    )
    ENDFOREACH()
ENDIF(EXEC_SOURCES)

ENDMACRO(OD_INIT_MODULE)

MACRO( OD_ADD_DEPS DEPLIST )

FOREACH( DEP ${DEPLIST} )

    #Check if dep is already in the list
    LIST(FIND ${LIB_NAME}_DEPS ${DEP} INDEX)
    IF(${INDEX} EQUAL -1)
	#Add dependency
	LIST(APPEND ${LIB_NAME}_DEPS ${DEP})
	
	#Add dependencies of dependencies
	FOREACH( DEPLIB ${${DEP}_DEPS} )
	    OD_ADD_DEPS( ${DEPLIB} )
	ENDFOREACH()

	#Add dependencies to include-path
	LIST(APPEND ${LIB_NAME}_INCLUDEPATH
		    ${OpendTect_SOURCE_DIR}/include/${DEP})
    ENDIF()
ENDFOREACH()

ENDMACRO(OD_ADD_DEPS)
