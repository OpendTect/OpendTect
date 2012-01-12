#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.4 2012-01-12 06:35:02 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    IF( ${CMAKE_BUILD_TYPE} MATCHES Debug)
        ADD_DEFINITIONS("-D__debug__")
    ENDIF()
    IF(APPLE)
        ADD_DEFINITIONS("-D__mac__ -Dmac")
        FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
        FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
        SET(EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
    ELSE()
        ADD_DEFINITIONS("-Dlux")
    ENDIF()
ENDIF(UNIX)

IF(WIN32)
    ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")
    SET(EXTRA_LIBS "ws2_32" "shlwapi")
ENDIF()

