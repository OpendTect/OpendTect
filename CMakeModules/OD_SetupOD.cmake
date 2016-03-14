SET( OD_CORE_SUBSYSTEM "od" )

SET( INCLUDES
    ODOpenCL
    ODSubversion
    ODQtUtils
    ODPlatformUtils
    ODUtils
    ODZlibUtils
    ODOsgUtils
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
