#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.1 2012-01-11 11:43:19 cvskris Exp $
#_______________________________________________________________________________

IF(APPLE)
    ADD_DEFINITIONS("-D__mac__ -Dmac")
    FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
    FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
    SET(EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
ELSEIF(WIN32)
ELSE()
    ADD_DEFINITIONS("-Dlux")
ENDIF()

