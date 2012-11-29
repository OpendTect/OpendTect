#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODUtils.cmake,v 1.20 2012/06/21 04:59:35 cvsraman Exp $
#_______________________________________________________________________________

IF ( CMAKE_BUILD_TYPE STREQUAL "" )
    SET ( DEBUGENV $ENV{DEBUG} )
    IF ( DEBUGENV AND
	( (${DEBUGENV} MATCHES "yes" ) OR
	  (${DEBUGENV} MATCHES "Yes" ) OR
	  (${DEBUGENV} MATCHES "YES" ) ) )
	SET ( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Debug or Release" FORCE )
    ELSE()
	SET ( CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug or Release" FORCE)
    ENDIF()

    MESSAGE( STATUS "Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}" )
ENDIF()

ADD_DEFINITIONS("-D__cmake__")

SET ( OD_EXEC_OUTPUT_RELPATH bin/${OD_PLFSUBDIR} )
SET ( OD_EXEC_OUTPUT_PATH ${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH} )
SET ( OD_EXEC_INSTALL_PATH ${OD_EXEC_OUTPUT_RELPATH} )

SET ( OD_MAIN_EXEC od_main )
SET ( OD_ATTRIB_EXECS od_process_attrib od_process_attrib_em od_stratamp )
SET ( OD_VOLUME_EXECS od_process_volume )
SET ( OD_PRESTACK_EXECS od_process_prestack )
SET ( OD_ZAXISTRANSFORM_EXECS od_process_time2depth )

#Macro for going through a list of modules and adding them
MACRO ( OD_ADD_MODULES )
    SET( DIR ${ARGV0} )

    FOREACH( OD_MODULE_NAME ${ARGV} )
	IF ( NOT ${OD_MODULE_NAME} MATCHES ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} )
	ENDIF()
    ENDFOREACH()

ENDMACRO()


# Macro for going through a list of modules and adding them
# as optional targets
MACRO ( OD_ADD_OPTIONAL_MODULES )
    SET( DIR ${ARGV0} )

    FOREACH( OD_MODULE_NAME ${ARGV} )
        IF ( NOT ${OD_MODULE_NAME} MATCHES ${DIR} )
            add_subdirectory( ${DIR}/${OD_MODULE_NAME} EXCLUDE_FROM_ALL )
        ENDIF()
    ENDFOREACH()
ENDMACRO()

macro ( OD_INSTALL_LIB ${ARGV} )
    set ( NEXT_ARG_IS_DEST )
    foreach ( ARG ${ARGS} )
	if ( ${ARG} STREQUAL "DESTINATION" )
	    set ( NEXT_ARG_IS_DEST 1 )
	else()
	    if ( NEXT_ARG_IS_DEST )
		set ( DST ${ARG} )
		set ( NEXT_ARG_IS_DEST )         
	    else()
		get_filename_component( FULLPATH ${ARG} REALPATH )
		list ( APPEND SOURCES ${FULLPATH} )
	    endif()
	endif()
    endforeach()
    INSTALL ( PROGRAMS ${SOURCES}
		DESTINATION ${DST}
		PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
			    OWNER_READ GROUP_READ WORLD_READ)
endmacro()
