#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( PLUGINS
	My_Plugin_Name
)

set( OD_PLUGINS
        CEEMDAttrib
        ExpAttribs
        GLCM
        GMT
        ODHDF5
        Hello
        Madagascar
        MadagascarAttribs
        MATLABLink
        Tut
        uiCEEMDAttrib
        uiSEGYTools
        uiMadagascarAttribs
	ODBind
)

if ( NOT OD_NO_QSQL )
    list( APPEND OD_PLUGINS ODQSql )
endif()

if ( NOT OD_NO_PROJ )
    list( APPEND OD_PLUGINS CRS uiCRS )
endif()

if ( NOT OD_NO_QPRINTSUPPORT )
    list( APPEND OD_PLUGINS uiODPrintSupport )
endif()

if ( NOT OD_NO_OSG )
    list( APPEND OD_PLUGINS
	uiCmdDriverMgr
        uiCOLOP
        uiCrashMe
        uiDPSDemo
        uiExpAttribs
        uiGLCM
        uiGMT
        uiGoogleIO
        uiHello
        uiImpGPR
        uiMadagascar
        uiMATLABLink
        uiPresentationMaker
        uiPreStackViewer
        uiQtApp
        uiSEGY
        uiTut
        uiTutMadagascar
    )
endif()

INCLUDE ( ODdGBPlugins.cmake OPTIONAL )

list( APPEND PLUGINS ${OD_PLUGINS} )
OD_ADD_MODULES( plugins ${PLUGINS} )
