SET( OD_CORE_SUBSYSTEM "od" )

SET( INCLUDES
    ODOpenCL
    ODSubversion
    ODQtUtils
    ODPlatformUtils
    ODUtils
    ODZlibUtils
    ODOpenSSLUtils
    ODProj4Utils
    ODBreakPadUtils
    ODMacroUtils
    ODModDeps
    CreateLaunchers
    ODAloFile
    ODInitheader
    ODDocumentation
    ODPackages
    ODTesting
    ODTranslation
    ODIttNotifyUtils
)

if ( NOT OD_NO_OSG )
    SET( INCLUDES ${INCLUDES} ODOsgUtils )
endif()

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()
