#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
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

macro( OD_ADD_ALO_ENTRIES )
    #Add OD_ALO_NAME to list 
    list( FIND OD_ALO_NAMES ${OD_ALO_NAME} INDEX )
    if( ${INDEX} EQUAL -1 )
	set ( OD_ALO_NAMES ${OD_ALO_NAMES} ${OD_ALO_NAME} )
	set ( OD_ALO_NAMES ${OD_ALO_NAMES} ${OD_ALO_NAME} PARENT_SCOPE )
    endif()

    foreach( EXEC ${ARGV} )
	#Add EXEC to list of execs
	list( FIND OD_ALO_FILE_EXECS_${OD_ALO_NAME} ${EXEC} INDEX )
	if( ${INDEX} EQUAL -1 )
	    set ( OD_ALO_FILE_EXECS_${OD_ALO_NAME}
		  ${OD_ALO_FILE_EXECS_${OD_ALO_NAME}} ${EXEC} )
	    set ( OD_ALO_FILE_EXECS_${OD_ALO_NAME}
		  ${OD_ALO_FILE_EXECS_${OD_ALO_NAME}} ${EXEC} PARENT_SCOPE )
	endif()

	if ( EXEC STREQUAL ${OD_MAIN_EXEC}) 
	    set( EXEC_IS_MAIN 1 )
	else()
	    set( EXEC_IS_MAIN 0 )
	endif()

	#Add all dependencies to alo-entry
	foreach( DEP ${OD_MODULE_INTERNAL_LIBS} )
	    list ( FIND OD_CORE_MODULE_NAMES_${OD_CORE_SUBSYSTEM} ${DEP} INDEX )
	    if ( ${INDEX} EQUAL -1 OR ${EXEC_IS_MAIN} EQUAL 0 )
		#Check that it is not already in the list
		list( FIND OD_ALO_FILE_ENTRIES_${EXEC} ${DEP} INDEX )
		if( ${INDEX} EQUAL -1 )
		    set ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
			  ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} ${DEP} )
		endif()
	    endif()
        endforeach()

	#Check if that noone has added this before 
	list( FIND OD_ALO_FILE_ENTRIES_${EXEC} ${OD_MODULE_NAME} INDEX )
	if( ${INDEX} EQUAL -1 )
	    set ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
		${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} ${OD_MODULE_NAME})
	endif()

	set ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
	  ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} )
	set ( OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}
	  ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} PARENT_SCOPE )
    endforeach()

endmacro()

# OD_WRITE_ALOFILES - Writes all alo-entries to alo-files
#
# Input variables
#
# OD_PLFSUBDIR				: lux64 or ....
# OD_ALO_FILE_EXECS_${OD_ALO_NAME}}	: list of executables (od_main ...)
# OD_OD_ALO_FILE_ENTRIES_${EXEC}	: list of deps for each executable

macro ( OD_WRITE_ALOFILES BASEDIR )
    foreach( OD_ALO_NAME ${OD_ALO_NAMES} )
	foreach( EXEC ${OD_ALO_FILE_EXECS_${OD_ALO_NAME}} )
	    set ( OD_ALOFILE ${BASEDIR}/${EXEC}.${OD_ALO_NAME}.alo )
	    set ( FIRST 1 )
	    foreach( ENTRY ${OD_ALO_FILE_ENTRIES_${OD_ALO_NAME}_${EXEC}} )
		if ( FIRST )
		    file( WRITE ${OD_ALOFILE} ${ENTRY} "\n" )
		    set ( FIRST )
		else()
		    file( APPEND ${OD_ALOFILE} ${ENTRY} "\n" )
		endif()
	    endforeach()
	endforeach()
    endforeach()
endmacro()
