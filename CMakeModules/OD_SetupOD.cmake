SET( OD_CORE_SUBSYSTEM "od" )

option ( OD_FORCE_QT5_UNSUPPORTED "Force Qt5, even though it is not supported in 6.x" NO )
set( OD_QT_INCLUDE ODQtUtils )
if ( OD_FORCE_QT5_UNSUPPORTED )
    set( OD_QT_INCLUDE OD6Qt5 )
endif()


SET( INCLUDES
    ODSubversion
    ODPlatformUtils
    ODUtils
    ${OD_QT_INCLUDE}
    ODZlibUtils
    ODOsgUtils
    ODPythonUtils
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

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()
