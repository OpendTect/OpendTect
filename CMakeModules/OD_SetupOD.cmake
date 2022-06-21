SET( OD_CORE_SUBSYSTEM "od" )

SET( INCLUDES
    ODSubversion
    ODPlatformUtils
    ODUtils
    ODQtUtils
    ODOsgUtils
    ODZlibUtils
    ODProjUtils
    ODOpenSSLUtils
    ODSqlite
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
