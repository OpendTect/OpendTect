#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODUtils.cmake,v 1.11 2012-02-24 14:54:45 cvskris Exp $
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

SET ( OD_PLUGIN_OUTPUT_RELPATH plugins/${OD_PLFSUBDIR}/libs )
SET ( OD_EXEC_OUTPUT_RELPATH bin/${OD_PLFSUBDIR}/${OD_OUTPUTDIR} )
SET ( OD_LIB_OUTPUT_RELPATH bin/${OD_PLFSUBDIR}/${OD_OUTPUTDIR}/so )

SET ( OD_PLUGIN_OUTPUT_PATH ${OpendTect_DIR}/${OD_PLUGIN_OUTPUT_RELPATH} )
SET ( OD_EXEC_OUTPUT_PATH ${OpendTect_DIR}/${OD_EXEC_OUTPUT_RELPATH} )
SET ( OD_LIB_OUTPUT_PATH ${OpendTect_DIR}/${OD_LIB_OUTPUT_RELPATH} )

#Macro for going through a list of modules and adding them
MACRO ( OD_ADD_MODULES )
    SET( DIR ${ARGV0} )

    FOREACH( OD_MODULE_NAME ${ARGV} )
	IF ( NOT ${OD_MODULE_NAME} MATCHES ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} )
	ENDIF()
    ENDFOREACH()

ENDMACRO()
