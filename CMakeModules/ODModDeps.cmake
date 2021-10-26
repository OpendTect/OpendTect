#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
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
install( FILES ${OD_MODDEPS_FILE} DESTINATION ${MISC_INSTALL_PREFIX}/data )

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
    file( APPEND ${OD_MODDEPS_FILE} ${OD_LINESEP} )
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

set( OD_FIND_OD_FILE ${CMAKE_BINARY_DIR}/CMakeModules/FindOpendTect.cmake )

install( FILES ${OD_FIND_OD_FILE} DESTINATION ${MISC_INSTALL_PREFIX}/data )

foreach ( MODULE ${OD_MODULE_NAMES_${OD_SUBSYSTEM}} )
    if ( OD_${MODULE}_DEPS )
	set( MODULE_DEPS
	    "${MODULE_DEPS}set( OD_${MODULE}_DEPS ${OD_${MODULE}_DEPS} )${OD_LINESEP}" )
    endif()

    if ( OD_${MODULE}_INCLUDEPATH )
	set ( PRINTPATH )
	foreach( INCLUDEPATH ${OD_${MODULE}_INCLUDEPATH} )
	    string( REPLACE ${CMAKE_SOURCE_DIR} "\${OpendTect_DIR}" INCLUDEPATH
				${INCLUDEPATH} )
	    string( REPLACE ${CMAKE_BINARY_DIR} "\${OD_BINARY_BASEDIR}" INCLUDEPATH
				${INCLUDEPATH} )
	    set( PRINTPATH "${PRINTPATH} ${INCLUDEPATH}" )
	endforeach()
	set ( MODULE_INCLUDES 
	   "${MODULE_INCLUDES}set( OD_${MODULE}_INCLUDEPATH ${PRINTPATH} )${OD_LINESEP}" )
    endif()

    if ( OD_${MODULE}_EXTERNAL_LIBS )
	set ( MODULE_EXTERNAL_LIBS 
	    "${MODULE_EXTERNAL_LIBS}set( OD_${MODULE}_EXTERNAL_LIBS ${OD_${MODULE}_EXTERNAL_LIBS} )${OD_LINESEP}" )
    endif()

    if ( OD_${MODULE}_EXTERNAL_RUNTIME_LIBS )
	set ( MODULE_EXTERNAL_RUNTIME_LIBS 
	    "${MODULE_EXTERNAL_RUNTIME_LIBS}set( OD_${MODULE}_EXTERNAL_RUNTIME_LIBS ${OD_${MODULE}_EXTERNAL_RUNTIME_LIBS} )${OD_LINESEP}" )
    endif()

    foreach ( SETUPNM ${SETUPNMS} )
	if ( OD_${MODULE}_SETUP_${SETUPNM} )
	    set( MODULES_SETUP
		"${MODULES_SETUP}set( OD_${MODULE}_${SETUPNM} ${OD_${MODULE}_SETUP_${SETUPNM}} )${OD_LINESEP}" )
	endif()
    endforeach()
endforeach()

if ( NOT OD_NO_OSG )
   OD_FIND_OSGDIR()
endif()
configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/FindOpendTect.cmake.in
		${OD_FIND_OD_FILE}
		@ONLY )
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

file( WRITE ${OD_PROJECT_FILE} "<Project name=\"OpendTect\">${OD_LINESEP}")
file( WRITE ${OD_SUBPROJECT_LISTFILE} "set ( CTEST_PROJECT_SUBPROJECTS${OD_LINESEP}")
foreach ( MODULE ${OD_MODULE_NAMES_${OD_SUBSYSTEM}} )
    if ( NOT ${MODULE} MATCHES "AllNonUi" )
	file( APPEND ${OD_PROJECT_FILE}
	    "    <SubProject name=\"${MODULE}\">${OD_LINESEP}")

	file( APPEND ${OD_SUBPROJECT_LISTFILE}
	    "    \"${MODULE}\"${OD_LINESEP}")

	#Add all module dependencies
	if( OD_${MODULE}_DEPS )
	    foreach( DEP ${OD_${MODULE}_DEPS} )
		file( APPEND ${OD_PROJECT_FILE}
		 "\t<Dependency name=\"${DEP}\">${OD_LINESEP}")
	    endforeach()
	endif()

	file( APPEND ${OD_PROJECT_FILE} "    </SubProject>${OD_LINESEP}")
    endif()
endforeach()
file( APPEND ${OD_PROJECT_FILE} "</Project>${OD_LINESEP}")
file( APPEND ${OD_SUBPROJECT_LISTFILE} ")${OD_LINESEP}" )

endmacro()
