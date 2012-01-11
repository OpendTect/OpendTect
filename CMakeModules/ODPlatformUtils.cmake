#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.3 2012-01-11 16:25:59 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    IF( ${CMAKE_BUILD_TYPE} MATCHES Debug)
        ADD_DEFINITIONS("-D__debug__")
    ENDIF()
ENDIF(UNIX)

IF(APPLE)
    ADD_DEFINITIONS("-D__mac__ -Dmac")
    FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
    FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
    SET(EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
ELSEIF(WIN32)
    ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")
ELSE()
    ADD_DEFINITIONS("-Dlux")
ENDIF()

