SET( OD_CMAKE_FILES
    ODPlatformUtils
    ODUtils
    ODQtUtils
    ODZlibUtils
    ODCoinUtils
    ODMacroUtils
    ODModDeps
    CreateLaunchers
    ODAloFile
    ODInitheader
    ODCodesign
    ODDoxygen
    ODPackages
)

FOREACH( INC ${OD_CMAKE_FILES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()

SET( OD_CORE_SUBSYSTEM "od" )
