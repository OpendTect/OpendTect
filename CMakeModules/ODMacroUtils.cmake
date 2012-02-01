#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODMacroUtils.cmake,v 1.8 2012-02-01 11:50:23 cvskris Exp $
#_______________________________________________________________________________

# Setups a number of variables for compiling OpendTect:
# OD_${MODULE_NAME}_INCLUDEPATH : The path(s) to the headerfiles of the module
#				  Normally one single one, but may have multiple
#				  paths in plugins
# OD_${MODULE_NAME}_DEPS	: The modules this module is dependent on
# MODULE_INCLUDEPATH		: The includepath needed to compile the source-
#				  files of the module

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

IF(OD_USECOIN)
    OD_SETUP_COIN()
ENDIF()

IF( OD_USEZLIB )
    LIST(APPEND MODULE_INCLUDEPATH ${ZLIB_INCLUDE_DIR} )
    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY} )
ENDIF( OD_USEZLIB )

IF(OD_USEOSG)
    OD_SETUP_OSG()
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
    LIST(APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
	${OpendTect_SOURCE_DIR}/include/${OD_MODULE_NAME} )
ENDIF(OD_IS_PLUGIN)

LIST(APPEND MODULE_INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )

#Export dependencies
SET( OD_${OD_MODULE_NAME}_DEPS ${OD_${OD_MODULE_NAME}_DEPS} PARENT_SCOPE )
SET( OD_${OD_MODULE_NAME}_INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} PARENT_SCOPE)

#Setup libraries & its deps
ADD_LIBRARY( ${OD_MODULE_NAME} SHARED ${OD_MODULE_SOURCES} ${QT_MOC_OUTFILES} )
TARGET_LINK_LIBRARIES(
        ${OD_MODULE_NAME}
        ${OD_MODULE_DEPS}
	${OD_MODULE_EXTERNAL_LIBS}
	${OD_MODULE_LINK_OPTIONS}
        ${EXTRA_LIBS} 
	${OD_QT_LIBS}
	${OD_COIN_LIBS}
	${OD_OSG_LIBS} )

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
    LIST(APPEND MODULE_INCLUDEPATH
		${OpendTect_SOURCE_DIR}/include/Prog)
ENDIF(OD_USE_PROG)

#Set current include_path
INCLUDE_DIRECTORIES( ${MODULE_INCLUDEPATH} )

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
    LIST(APPEND MODULE_INCLUDEPATH ${OD_${DEP}_INCLUDEPATH} )
ENDIF()

ENDMACRO(OD_ADD_DEPS)
