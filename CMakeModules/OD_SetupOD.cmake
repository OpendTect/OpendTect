SET( OD_CORE_SUBSYSTEM "od" )

SET( INCLUDES
    ODPlatformUtils
    ODUtils
    ODQtUtils
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
    ODIttNotifyUtils
)

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()

