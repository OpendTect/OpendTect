SET( OD_CORE_SUBSYSTEM "od" )

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
    ODDoxygen
    ODPackages
)

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()

