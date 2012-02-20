#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODUtils.cmake,v 1.6 2012-02-20 15:11:41 cvskris Exp $
#_______________________________________________________________________________

#Discover 64 or 32 bits
IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET( OD_64BIT 1 )
ENDIF()

#Discover 64 or 32 bits
IF( ${CMAKE_BUILD_TYPE} MATCHES Debug)
    SET( OD_DEBUG 1 )
    SET( OD_OUTPUTDIR "G" )
ELSE()
    SET( OD_DEBUG )
    SET( OD_OUTPUTDIR "O" )
ENDIF()

#Macro for going through a list of modules and adding them
MACRO ( OD_ADD_MODULES )
    SET( DIR ${ARGV0} )

    FOREACH( OD_MODULE_NAME ${ARGV} )
	IF ( NOT ${OD_MODULE_NAME} MATCHES ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} )
	ENDIF()
    ENDFOREACH()

ENDMACRO()

