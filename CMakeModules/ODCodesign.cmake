#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODCodesign.cmake,v 1.1 2012/03/20 09:25:04 cvskris Exp $
#_______________________________________________________________________________

SET ( OD_SIGN_ID "" CACHE STRING "Signing id" )

#Adds target codesign_install 
MACRO ( OD_SIGN_TARGETS )
    IF ( OD_SIGN_FILES )
	IF ( APPLE )
	    add_custom_target( sign_install_internal make install )
	    add_custom_target( codesign_install codesign -f -s ${OD_SIGN_ID} ${OD_SIGN_FILES}
			DEPENDS sign_install_internal
			COMMENT "Signing executables"
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${OD_EXEC_INSTALL_PATH}"
			VERBATIM )
	ENDIF ( APPLE )
    ENDIF( OD_SIGN_FILES )
ENDMACRO ( OD_SIGN_TARGETS )


MACRO( OD_SIGN_TARGET SIGN_TARGET )
    IF ( NOT OD_SIGN_COMMAND STREQUAL "" )
	get_target_property( TARGETPATH ${SIGN_TARGET} LOCATION )
	GET_FILENAME_COMPONENT( TARGETNAME ${TARGETPATH} NAME )
	
	SET ( OD_SIGN_FILES ${OD_SIGN_FILES} ${TARGETNAME} PARENT_SCOPE )
    ENDIF()

ENDMACRO()

