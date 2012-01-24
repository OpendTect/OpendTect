#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODOsgUtils.cmake,v 1.1 2012-01-24 14:09:57 cvskris Exp $
#_______________________________________________________________________________

SET( OD_OSGDIR_ENV $ENV{OD_OSGDIR})

IF(OD_OSGDIR_ENV)
    SET(OSGDIR ${OD_OSGDIR_ENV})
ELSE()
    SET(OSGDIR "" CACHE PATH "OSG location")
ENDIF()

MACRO(OD_SETUP_OSG)
    IF(OD_USEOSG)
        LIST(APPEND MODULE_INCLUDEPATH ${OSGDIR}/include )
	SET(OSGMODULES
		osg
		osgDB
		osgGA
		osgUtil
		osgQt
		osgWidget
		osgViewer
		osgVolume
		OpenThreads
		osgGeo )

	FOREACH( OSGMODULE ${OSGMODULES} )
	    FIND_LIBRARY(${OSGMODULE}_OSGLIB NAMES ${OSGMODULE} PATHS ${OSGDIR}/lib REQUIRED )
	    LIST(APPEND OD_OSG_LIBS ${${OSGMODULE}_OSGLIB} )
	ENDFOREACH()
    ENDIF()

ENDMACRO(OD_SETUP_OSG)
