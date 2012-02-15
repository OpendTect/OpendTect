#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODModDeps.cmake,v 1.1 2012-02-15 15:37:41 cvskris Exp $
#_______________________________________________________________________________

SET( OD_MODDEPS_FILE ${CMAKE_BINARY_DIR}/Pmake/ModDeps.od )

# OD_WRITE_MODDEP - Marcro that writes all modules and their dependencies to
#		    a file.
# Input variables:
# OD_MODULE_NAMES			: List of all modules.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.

MACRO( OD_WRITE_MODDEPS )

FILE(WRITE ${OD_MODDEPS_FILE} "")

FOREACH ( MODULE ${OD_MODULE_NAMES} )
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

