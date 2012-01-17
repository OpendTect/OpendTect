#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODMacroUtils.cmake,v 1.5 2012-01-17 16:23:04 cvskris Exp $
#_______________________________________________________________________________

MACRO( OD_INIT_MODULE )

#Add all module dependencies
IF(OD_MODULE_DEPS)
    FOREACH( DEP ${OD_MODULE_DEPS} )
	OD_ADD_DEPS( ${DEP} )
    ENDFOREACH()
ENDIF()

#Add Qt-stuff
IF(OD_USEQT)
   OD_SETUP_QT()
ENDIF(OD_USEQT)

#Add current module to include-path
IF (OD_IS_PLUGIN)
    SET(PLUGINDIR ${OpendTect_SOURCE_DIR}/plugins/${OD_MODULE_NAME})
    LIST(APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH ${PLUGINDIR})
    FOREACH( PLUGINSUBDIR ${PLUGINSUBDIRS} )
	LIST(APPEND ${OD_MODULE_NAME}_INCLUDEPATH
	    ${PLUGINDIR}/include/${PLUGINSUBDIR})
	INCLUDE(${PLUGINDIR}/src/${PLUGINSUBDIR}/CMakeFile.txt)
    ENDFOREACH()
ELSE()
    SET( OD_${OD_MODULE_NAME}_INCLUDEPATH
	${OpendTect_SOURCE_DIR}/include/${OD_MODULE_NAME} )
ENDIF(OD_IS_PLUGIN)

LIST(APPEND INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )

#Export dependencies
SET( OD_${OD_MODULE_NAME}_DEPS ${OD_${OD_MODULE_NAME}_DEPS} PARENT_SCOPE )
SET( OD_${OD_MODULE_NAME}_INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} PARENT_SCOPE)

#Setup libraries & its deps
ADD_LIBRARY( ${OD_MODULE_NAME} SHARED ${OD_MODULE_SOURCES} ${QT_MOC_OUTFILES} )
TARGET_LINK_LIBRARIES(
        ${OD_MODULE_NAME}
        ${OD_MODULE_DEPS}
        ${EXTRA_LIBS} 
	${OWN_QTLIBS} )

#Add executable targets
IF(OD_MODULE_EXECS)
    FOREACH( EXEC ${OD_MODULE_EXECS} )
	ADD_EXECUTABLE( ${EXEC} ${EXEC}.cc )
	TARGET_LINK_LIBRARIES(
	    ${EXEC}
	    ${OD_MODULE_NAME}
	    ${OD_MODULE_DEPS})
    ENDFOREACH()

    LIST(APPEND INCLUDEPATH
		${OpendTect_SOURCE_DIR}/include/Prog)

ENDIF(OD_MODULE_EXECS)

#Set current include_path
INCLUDE_DIRECTORIES( ${INCLUDEPATH} )

ENDMACRO(OD_INIT_MODULE)

MACRO( OD_ADD_DEPS DEP )
#Check if dep is already in the list
LIST(FIND OD_${OD_MODULE_NAME}_DEPS ${DEP} INDEX)
IF(${INDEX} EQUAL -1)
    #Add dependency
    LIST(APPEND OD_${OD_MODULE_NAME}_DEPS ${DEP})
    
    #Add dependencies of dependencies
    FOREACH( DEPLIB ${OD_${DEP}_DEPS} )
	OD_ADD_DEPS( ${DEPLIB} )
    ENDFOREACH()

    #Add dependencies to include-path
    LIST(APPEND INCLUDEPATH ${OD_${DEP}_INCLUDEPATH} )
ENDIF()

ENDMACRO(OD_ADD_DEPS)
