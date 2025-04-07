#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

SET( OD_CORE_SUBSYSTEM "od" )

SET( INCLUDES
    ODSubversion
    ODPlatformUtils
    ODUtils
    ODQtUtils
    ODZlibUtils
    ODOsgUtils
    ODProjUtils
    ODOpenSSLUtils
    ODSqliteUtils
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
    if ( EXISTS "${OpendTect_DIR}/CMakeModules/${INC}.cmake" )
        INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
    elseif ( EXISTS "${OpendTect_DIR}/Contents/Resources/CMakeModules/${INC}.cmake" )
        INCLUDE( ${OpendTect_DIR}/Contents/Resources/CMakeModules/${INC}.cmake )
    endif()
ENDFOREACH()
