#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

# OD_WRITE_MODDEP - Marcro that writes all modules and their dependencies to
#		    a file. 
# Input variables:
# OD_SUBSYSTEM				: "od" or "dgb"
# OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}	: List of all modules.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_INCLUDEPATH	: The include directories for each module

macro( OD_WRITE_MODDEPS BASEDIR )

set( OD_MODDEPS_FILE ${BASEDIR}/ModDeps.${OD_SUBSYSTEM} )
install( FILES ${OD_MODDEPS_FILE} DESTINATION data )

list( APPEND OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM} "AllNonUi" )
set( OD_AllNonUi_DEPS MPEEngine WellAttrib VolumeProcessing )


file( WRITE ${OD_MODDEPS_FILE} "")
foreach ( MODULE ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )
    #Start write ModDeps-line
    file( APPEND ${OD_MODDEPS_FILE}
	"${MODULE}:\t\tS.${MODULE}")

    #Add all module dependencies
    if( OD_${MODULE}_DEPS )
	foreach( DEP ${OD_${MODULE}_DEPS} )
	    file( APPEND ${OD_MODDEPS_FILE}
	     " D.${DEP}")
	endforeach()
    endif()

    #End ModDeps-line
    file( APPEND ${OD_MODDEPS_FILE} "\n")
endforeach()



endmacro()

# OD_WRITE_FINDFILE - Marcro that writes all modules and their dependencies to
#		      a file that can be read by pluginmakers
# Input variables:
# OD_SUBSYSTEM				: "od" or "dgb"
# OD_MODULE_NAMES_${OD_SUBSYSTEM}	: List of all modules.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_INCLUDEPATH	: The include directories for each module

macro( OD_WRITE_FINDFILE )

set( OD_FIND_OD_FILE ${CMAKE_SOURCE_DIR}/CMakeModules/FindOpendTect.cmake )

install( FILES ${OD_FIND_OD_FILE} DESTINATION data )
file( WRITE ${OD_FIND_OD_FILE} "SET( OpendTect_VERSION_MAJOR ${OpendTect_VERSION_MAJOR} )\n")
file( APPEND ${OD_FIND_OD_FILE} "SET( OpendTect_VERSION_MINOR ${OpendTect_VERSION_MINOR} )\n")
file( APPEND ${OD_FIND_OD_FILE} "SET( OpendTect_VERSION_DETAIL ${OpendTect_VERSION_DETAIL} )\n")
file( APPEND ${OD_FIND_OD_FILE} "SET( OD_BINARY_BASEDIR \"${OD_BINARY_BASEDIR}\" )\n" )
file( APPEND ${OD_FIND_OD_FILE} "INCLUDE( \${OpendTect_DIR}/CMakeModules/OD_SetupOD.cmake )\n")
file( APPEND ${OD_FIND_OD_FILE} "LINK_DIRECTORIES( \${OD_BINARY_BASEDIR}/\${OD_EXEC_OUTPUT_RELPATH} )\n")
file( APPEND ${OD_FIND_OD_FILE} "set( OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM} ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )\n" )

foreach ( MODULE ${OD_MODULE_NAMES_${OD_SUBSYSTEM}} )
    if ( OD_${MODULE}_DEPS )
	file( APPEND ${OD_FIND_OD_FILE}
	    "set( OD_${MODULE}_DEPS ${OD_${MODULE}_DEPS} )\n" )
    endif()

    if ( OD_${MODULE}_INCLUDEPATH )
	string( REPLACE ${CMAKE_SOURCE_DIR} "" INCLUDEPATH
			${OD_${MODULE}_INCLUDEPATH} )
	file( APPEND ${OD_FIND_OD_FILE}
	   "set( OD_${MODULE}_INCLUDEPATH \${OpendTect_DIR}${INCLUDEPATH} )\n" )
    endif()
    #if ( OD_${MODULE}_RUNTIMEPATH )
	#string( REPLACE ${CMAKE_SOURCE_DIR} "" RUNTIMEPATH
			#${OD_${MODULE}_RUNTIMEPATH} )
	#file( APPEND ${OD_FIND_OD_FILE}
       #"set( OD_${MODULE}_RUNTIMEPATH \${}${RUNTIMEPATH} )\n" )
    #endif()
endforeach()
endmacro()


# OD_WRITE_TEST_PROJECT_DESC - Marcro that writes an xml-file for cdash submition
#		    a file. 
# Input variables:
# OD_MODULE_NAMES_${OD_SUBSYSTEM}	: List of all modules.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_INCLUDEPATH	: The include directories for each module

macro( OD_WRITE_TEST_PROJECT_DESC BASEDIR )

set( OD_PROJECT_FILE ${BASEDIR}/Project.xml )
set( OD_SUBPROJECT_LISTFILE ${BASEDIR}/subprojects.cmake )

file( WRITE ${OD_PROJECT_FILE} "<Project name=\"OpendTect\">\n")
file( WRITE ${OD_SUBPROJECT_LISTFILE} "set ( CTEST_PROJECT_SUBPROJECTS\n")
foreach ( MODULE ${OD_MODULE_NAMES_${OD_SUBSYSTEM}} )
    if ( NOT ${MODULE} MATCHES "AllNonUi" )
	file( APPEND ${OD_PROJECT_FILE}
	    "    <SubProject name=\"${MODULE}\">\n")

	file( APPEND ${OD_SUBPROJECT_LISTFILE}
	    "    \"${MODULE}\"\n")

	#Add all module dependencies
	if( OD_${MODULE}_DEPS )
	    foreach( DEP ${OD_${MODULE}_DEPS} )
		file( APPEND ${OD_PROJECT_FILE}
		 "\t<Dependency name=\"${DEP}\">\n")
	    endforeach()
	endif()

	file( APPEND ${OD_PROJECT_FILE} "    </SubProject>\n")
    endif()
endforeach()
file( APPEND ${OD_PROJECT_FILE} "</Project>\n")
file( APPEND ${OD_SUBPROJECT_LISTFILE} ")\n" )

endmacro()
