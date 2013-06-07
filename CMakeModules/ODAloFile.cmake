#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODAloFile.cmake,v 1.5 2012/03/28 13:51:33 cvskris Exp $
#_______________________________________________________________________________


# OD_ADD_ALO_ENTRIES( executables ) - Records all alo-entries
#
# Input variables
#
# OD_ALO_NAME				: od or dgb or ....
# OD_PLFSUBDIR				: lux64 or ....
# ######
# Output variables
# OD_ALO_NAMES				: list of alo-names
# OD_ALO_FILE_EXECS_${OD_ALO_NAME}}	: list of executables (od_main ...)
# OD_OD_ALO_FILE_ENTRIES_${EXEC}	: list of deps for each executable

MACRO( OD_ADD_ALO_ENTRIES )
    #Add OD_ALO_NAME to list 
    LIST( FIND OD_ALO_NAMES ${OD_ALO_NAME} INDEX )
    IF( ${INDEX} EQUAL -1 )
	SET ( OD_ALO_NAMES ${OD_ALO_NAMES} ${OD_ALO_NAME} )
	SET ( OD_ALO_NAMES ${OD_ALO_NAMES} ${OD_ALO_NAME} PARENT_SCOPE )
    ENDIF()

    FOREACH( EXEC ${ARGV} )
	#Add EXEC to list of execs
	LIST( FIND OD_ALO_FILE_EXECS_${OD_ALO_NAME} ${EXEC} INDEX )
	IF( ${INDEX} EQUAL -1 )
	    SET ( OD_ALO_FILE_EXECS_${OD_ALO_NAME} ${OD_ALO_FILE_EXECS_${OD_ALO_NAME}} ${EXEC} )
	    SET ( OD_ALO_FILE_EXECS_${OD_ALO_NAME} ${OD_ALO_FILE_EXECS_${OD_ALO_NAME}} ${EXEC} PARENT_SCOPE )
	ENDIF()

	IF ( EXEC STREQUAL ${OD_MAIN_EXEC}) 
	    SET( EXEC_IS_MAIN 1 )
	ELSE()
	    SET( EXEC_IS_MAIN 0 )
	ENDIF()

	#Add all dependencies to alo-entry
	FOREACH( DEP ${OD_MODULE_INTERNAL_LIBS} )
	    LIST ( FIND OD_CORE_MODULE_NAMES_${OD_CORE_SUBSYSTEM} ${DEP} INDEX )
	    IF ( ${INDEX} EQUAL -1 OR ${EXEC_IS_MAIN} EQUAL 0 )
		#Check that it is not already in the list
		LIST( FIND OD_ALO_FILE_ENTRIES_${EXEC} ${DEP} INDEX )
		IF( ${INDEX} EQUAL -1 )
		    SET ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
			  ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} ${DEP} )
		ENDIF()
	    ENDIF()
        ENDFOREACH()

	#Check if that noone has added this before 
	LIST( FIND OD_ALO_FILE_ENTRIES_${EXEC} ${OD_MODULE_NAME} INDEX )
	IF( ${INDEX} EQUAL -1 )
	    SET ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
		${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} ${OD_MODULE_NAME} )
	ENDIF()

	SET ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
	  ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} )
	SET ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
	  ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} PARENT_SCOPE )
    ENDFOREACH()

ENDMACRO()

# OD_WRITE_ALOFILES - Writes all alo-entries to alo-files
#
# Input variables
#
# OD_PLFSUBDIR				: lux64 or ....
# OD_ALO_FILE_EXECS_${OD_ALO_NAME}}	: list of executables (od_main ...)
# OD_OD_ALO_FILE_ENTRIES_${EXEC}	: list of deps for each executable

MACRO ( OD_WRITE_ALOFILES BASEDIR )
    FOREACH( OD_ALO_NAME ${OD_ALO_NAMES} )
	FOREACH( EXEC ${OD_ALO_FILE_EXECS_${OD_ALO_NAME}} )
	    SET ( OD_ALOFILE ${BASEDIR}/${EXEC}.${OD_ALO_NAME}.alo )
	    SET ( FIRST 1 )
	    FOREACH( ENTRY ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} )
		IF ( FIRST )
		    FILE( WRITE ${OD_ALOFILE} ${ENTRY} "\n" )
		    SET ( FIRST )
		ELSE()
		    FILE( APPEND ${OD_ALOFILE} ${ENTRY} "\n" )
		ENDIF()
	    ENDFOREACH()
	ENDFOREACH()
    ENDFOREACH()
ENDMACRO()
