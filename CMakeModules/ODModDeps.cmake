#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODModDeps.cmake,v 1.12 2012/06/25 12:21:19 cvskris Exp $
#_______________________________________________________________________________

# OD_WRITE_MODDEP - Marcro that writes all modules and their dependencies to
#		    a file. 
# Input variables:
# OD_SUBSYSTEM				: "od" or "dgb"
# OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}	: List of all modules.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_INCLUDEPATH	: The include directories for each module

MACRO( OD_WRITE_MODDEPS BASEDIR )

SET( OD_MODDEPS_FILE ${BASEDIR}/ModDeps.${OD_SUBSYSTEM} )
INSTALL( FILES ${OD_MODDEPS_FILE} DESTINATION data )

LIST( APPEND OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM} "AllNonUi" )
SET( OD_AllNonUi_DEPS MPEEngine WellAttrib VolumeProcessing )


FILE(WRITE ${OD_MODDEPS_FILE} "")
FOREACH ( MODULE ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )
    #Start write ModDeps-line
    FILE(APPEND ${OD_MODDEPS_FILE}
	"${MODULE}:\t\tS.${MODULE}")

    #Add all module dependencies
    IF( OD_${MODULE}_DEPS )
	FOREACH( DEP ${OD_${MODULE}_DEPS} )
	    FILE(APPEND ${OD_MODDEPS_FILE}
	     " D.${DEP}")
	ENDFOREACH()
    ENDIF()

    #End ModDeps-line
    FILE(APPEND ${OD_MODDEPS_FILE} "\n")
ENDFOREACH()



ENDMACRO()

# OD_WRITE_FINDFILE - Marcro that writes all modules and their dependencies to
#		      a file that can be read by pluginmakers
# Input variables:
# OD_SUBSYSTEM				: "od" or "dgb"
# OD_MODULE_NAMES_${OD_SUBSYSTEM}	: List of all modules.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_INCLUDEPATH	: The include directories for each module

MACRO( OD_WRITE_FINDFILE )

SET( OD_FIND_OD_FILE ${CMAKE_SOURCE_DIR}/CMakeModules/FindOpendTect.cmake )
FILE( WRITE ${OD_FIND_OD_FILE} "set ( OD_BINARY_BASEDIR \"${OD_BINARY_BASEDIR}\" )\n" )
FILE( APPEND ${OD_FIND_OD_FILE} "INCLUDE ( \${OpendTect_DIR}/CMakeModules/OD_SetupOD.cmake )\n")
FILE( APPEND ${OD_FIND_OD_FILE} "LINK_DIRECTORIES ( \${OD_BINARY_BASEDIR}/\${OD_EXEC_OUTPUT_RELPATH} )\n")
FILE( APPEND ${OD_FIND_OD_FILE} "SET ( OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM} ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )\n" )

FOREACH ( MODULE ${OD_MODULE_NAMES_${OD_SUBSYSTEM}} )
    IF ( OD_${MODULE}_DEPS )
	FILE(APPEND ${OD_FIND_OD_FILE}
	    "SET( OD_${MODULE}_DEPS ${OD_${MODULE}_DEPS} )\n" )
    ENDIF()

    IF ( OD_${MODULE}_INCLUDEPATH )
	STRING( REPLACE ${CMAKE_SOURCE_DIR} "" INCLUDEPATH
			${OD_${MODULE}_INCLUDEPATH} )
	FILE(APPEND ${OD_FIND_OD_FILE}
	   "SET( OD_${MODULE}_INCLUDEPATH \${OpendTect_DIR}${INCLUDEPATH} )\n" )
    ENDIF()
    #IF ( OD_${MODULE}_RUNTIMEPATH )
	#STRING( REPLACE ${CMAKE_SOURCE_DIR} "" RUNTIMEPATH
			#${OD_${MODULE}_RUNTIMEPATH} )
	#FILE(APPEND ${OD_FIND_OD_FILE}
       #"SET( OD_${MODULE}_RUNTIMEPATH \${}${RUNTIMEPATH} )\n" )
    #ENDIF()
ENDFOREACH()
ENDMACRO()
