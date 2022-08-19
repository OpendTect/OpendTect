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
