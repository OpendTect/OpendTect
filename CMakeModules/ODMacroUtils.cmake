#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODMacroUtils.cmake,v 1.6 2012-01-23 20:53:35 cvskris Exp $
#_______________________________________________________________________________


MACRO( OD_INIT_MODULE )

#Start write ModDeps-line
FILE(APPEND ${OpendTect_SOURCE_DIR}/Pmake/ModDeps.od
    "${OD_MODULE_NAME}:\t\tS.${OD_MODULE_NAME}")

#Add all module dependencies
IF(OD_MODULE_DEPS)
    FOREACH( DEP ${OD_MODULE_DEPS} )
	OD_ADD_DEPS( ${DEP} )
	FILE(APPEND ${OpendTect_SOURCE_DIR}/Pmake/ModDeps.od
	 " D.${DEP}")
    ENDFOREACH()
ENDIF()

#End ModDeps-line
FILE(APPEND ${OpendTect_SOURCE_DIR}/Pmake/ModDeps.od "\n")

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

    SET(OD_USE_PROG 1)
ENDIF(OD_MODULE_EXECS)

IF(OD_MODULE_BATCHPROGS)
    FOREACH( EXEC ${OD_MODULE_BATCHPROGS} )
	ADD_EXECUTABLE( ${EXEC} ${EXEC}.cc )
	TARGET_LINK_LIBRARIES(
	    ${EXEC}
	    ${OD_MODULE_NAME}
	    "Batch"
	    ${OD_MODULE_DEPS})
	SET_PROPERTY( TARGET ${EXEC} PROPERTY COMPILE_DEFINITIONS __prog__ )
    ENDFOREACH()

    SET(OD_USE_PROG 1)
ENDIF(OD_MODULE_BATCHPROGS)

IF(OD_USE_PROG)
    LIST(APPEND INCLUDEPATH
		${OpendTect_SOURCE_DIR}/include/Prog)
ENDIF(OD_USE_PROG)

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
