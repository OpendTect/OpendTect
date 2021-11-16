#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       RCS :           $Id: RootMakefile,v 1.4 2010-09-28 07:21:07 cvsbert
#_______________________________________________________________________________

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
        HorizonAttrib
        Madagascar
        MadagascarAttribs
        MATLABLink
        Tut
        uiCEEMDAttrib
        uiSEGYTools
        uiMadagascarAttribs
)

if ( NOT OD_NO_OSG )
    list( APPEND OD_PLUGINS
        CmdDriver
        uiCOLOP
        uiCrashMe
        uiDPSDemo
        uiExpAttribs
        uiGLCM
        uiGMT
        uiGoogleIO
        uiHello
        uiHorizonAttrib
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

QT_DTECT_CHARTS()
if ( Qt5Charts_FOUND )
    OD_ADD_MODULES( external/ODCharts/plugins uiCharts uiWellCharts )
    list( APPEND PLUGINS uiCharts;uiWellCharts )
endif()
