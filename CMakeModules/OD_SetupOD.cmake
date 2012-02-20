SET( INCLUDES
    OD_DEPS 
    ODUtils
    ODQtUtils
    ODZlibUtils
    ODCoinUtils
    ODOsgUtils
    ODPlatformUtils
    ODMacroUtils
    ODModDeps
    CreateLaunchers
)

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()
