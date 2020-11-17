SET( OD_CORE_SUBSYSTEM "od" )

SET( INCLUDES
    ODOpenCL
    ODSubversion
    ODPlatformUtils
    ODUtils
    ODQtUtils
    ODOsgUtils
    ODZlibUtils
    ODProj4Utils
    ODOpenSSLUtils
    ODHDF5Utils
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

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()
