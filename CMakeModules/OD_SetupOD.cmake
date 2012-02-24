SET( INCLUDES
    ODPlatformUtils
    ODUtils
    ODQtUtils
    ODZlibUtils
    ODCoinUtils
    ODOsgUtils
    ODMacroUtils
    ODModDeps
    CreateLaunchers
    ODAloFile
    ODInitheader
)

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()

SET( OD_CORE_SUBSYSTEM "od" )
