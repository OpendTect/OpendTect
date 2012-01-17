#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODMacroUtils.cmake,v 1.4 2012-01-17 05:44:11 cvskris Exp $
#_______________________________________________________________________________

MACRO( OD_INIT_MODULE )

#Add all module dependencies
IF(DEPS)
    FOREACH( DEP ${DEPS} )
	OD_ADD_DEPS( ${DEP} )
    ENDFOREACH()
ENDIF()

#Add Qt-stuff
IF(OD_USEQT)
   OD_SETUP_QT()
ENDIF(OD_USEQT)

#Add current module to include-path
IF (OD_IS_PLUGIN)
    SET(PLUGINDIR ${OpendTect_SOURCE_DIR}/plugins/${LIB_NAME})
    LIST(APPEND ${LIB_NAME}_INCLUDEPATH ${PLUGINDIR})
    FOREACH( PLUGINSUBDIR ${PLUGINSUBDIRS} )
	LIST(APPEND ${LIB_NAME}_INCLUDEPATH
	    ${PLUGINDIR}/include/${PLUGINSUBDIR})
	INCLUDE(${PLUGINDIR}/src/${PLUGINSUBDIR}/CMakeFile.txt)
    ENDFOREACH()
ELSE()
    SET( ${LIB_NAME}_INCLUDEPATH
	${OpendTect_SOURCE_DIR}/include/${LIB_NAME} )
ENDIF(OD_IS_PLUGIN)

LIST(APPEND INCLUDEPATH ${${LIB_NAME}_INCLUDEPATH} )

#Export dependencies
SET( ${LIB_NAME}_DEPS ${${LIB_NAME}_DEPS} PARENT_SCOPE )
SET( ${LIB_NAME}_INCLUDEPATH ${${LIB_NAME}_INCLUDEPATH} PARENT_SCOPE)

#Setup libraries & its deps
ADD_LIBRARY( ${LIB_NAME} SHARED ${LIB_SOURCES} ${QT_MOC_OUTFILES} )
TARGET_LINK_LIBRARIES(
        ${LIB_NAME}
        ${DEPS}
        ${EXTRA_LIBS} 
	${OWN_QTLIBS} )

#Add executable targets
IF(EXEC_SOURCES)
    FOREACH( EXEC ${EXEC_SOURCES} )
	ADD_EXECUTABLE( ${EXEC} ${EXEC}.cc )
	TARGET_LINK_LIBRARIES(
	    ${EXEC}
	    ${LIB_NAME}
	    ${DEPS})
    ENDFOREACH()

    LIST(APPEND INCLUDEPATH
		${OpendTect_SOURCE_DIR}/include/Prog)

ENDIF(EXEC_SOURCES)

#Set current include_path
INCLUDE_DIRECTORIES( ${INCLUDEPATH} )

ENDMACRO(OD_INIT_MODULE)

MACRO( OD_ADD_DEPS DEP )
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
    LIST(APPEND INCLUDEPATH ${${DEP}_INCLUDEPATH} )
ENDIF()

ENDMACRO(OD_ADD_DEPS)
