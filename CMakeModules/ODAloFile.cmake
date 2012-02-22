#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODAloFile.cmake,v 1.1 2012-02-22 09:07:25 cvskris Exp $
#_______________________________________________________________________________


# OD_ADD_ALO_ENTRIES( executables ) - Records all alo-entries
#
# Input variables
#
# OD_SUBSYSTEM				: od or dgb or ....
# OD_PLFSUBDIR				: lux64 or ....
# ######
# Output variables
# OD_ALO_FILE_EXECS_${OD_SUBSYSTEM}}	: list of executables (od_main ...)
# OD_OD_ALO_FILE_ENTRIES_${EXEC}	: list of deps for each executable

MACRO( OD_ADD_ALO_ENTRIES )
    FOREACH( EXEC ${ARGV} )
	#Add EXEC to list of execs
	LIST( FIND OD_ALO_FILE_EXECS_${OD_SUBSYSTEM} ${EXEC} INDEX )
	IF( ${INDEX} EQUAL -1 )
	    SET ( OD_ALO_FILE_EXECS_${OD_SUBSYSTEM} ${OD_ALO_FILE_EXECS_${OD_SUBSYSTEM}} ${EXEC} PARENT_SCOPE )
	ENDIF()

	#Add all dependencies to alo-entry
	FOREACH( DEP ${OD_MODULE_INTERNAL_LIBS} )
	    # Check that dep is not a core-library
            LIST( FIND OD_CORE_MODULE_NAMES_${OD_CORE_SUBSYSTEM} ${DEP} INDEX )
            IF( ${INDEX} EQUAL -1 )
		#Check that it is not already in the list
		LIST( FIND OD_ALO_FILE_ENTRIES_${EXEC} ${DEP} INDEX )
		IF( ${INDEX} EQUAL -1 )
		    SET ( OD_ALO_FILE_ENTRIES_${EXEC}
			  ${OD_ALO_FILE_ENTRIES_${EXEC}} ${DEP} )
		ENDIF()
            ENDIF()
        ENDFOREACH()

	# Check that lib is not a core-library
	LIST( FIND OD_CORE_MODULE_NAMES_${OD_CORE_SUBSYSTEM} ${OD_MODULE_NAME} INDEX )
	IF( ${INDEX} EQUAL -1 )
	    #Check if that noone has added this before 
	    LIST( FIND OD_ALO_FILE_ENTRIES_${EXEC} ${OD_MODULE_NAME} INDEX )
	    IF( ${INDEX} EQUAL -1 )
		SET ( OD_ALO_FILE_ENTRIES_${EXEC}
		    ${OD_ALO_FILE_ENTRIES_${EXEC}} ${OD_MODULE_NAME} )
	    ENDIF()
	ENDIF()

	SET ( OD_ALO_FILE_ENTRIES_${EXEC}
	  ${OD_ALO_FILE_ENTRIES_${EXEC}} PARENT_SCOPE )
    ENDFOREACH()

ENDMACRO()

# OD_WRITE_ALOFILES - Writes all alo-entries to alo-files
#
# Input variables
#
# OD_SUBSYSTEM				: od or dgb or ....
# OD_PLFSUBDIR				: lux64 or ....
# OD_ALO_FILE_EXECS_${OD_SUBSYSTEM}}	: list of executables (od_main ...)
# OD_OD_ALO_FILE_ENTRIES_${EXEC}	: list of deps for each executable

MACRO ( OD_WRITE_ALOFILES )
    FOREACH( EXEC ${OD_ALO_FILE_EXECS_${OD_SUBSYSTEM}} )
	SET ( OD_ALOFILE ${OD_BINARY_BASEDIR}/plugins/${OD_PLFSUBDIR}/${EXEC}.${OD_SUBSYSTEM}.alo )
	SET ( FIRST 1 )
	FOREACH( ENTRY ${OD_ALO_FILE_ENTRIES_${EXEC}} )
	    IF ( FIRST )
		FILE( WRITE ${OD_ALOFILE} ${ENTRY} "\n" )
		SET ( FIRST )
	    ELSE()
		FILE( APPEND ${OD_ALOFILE} ${ENTRY} "\n" )
	    ENDIF()
	ENDFOREACH()
    ENDFOREACH()
ENDMACRO()
