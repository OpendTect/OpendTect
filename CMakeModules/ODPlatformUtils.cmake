#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.7 2012-01-17 05:44:12 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    IF( ${CMAKE_BUILD_TYPE} MATCHES Debug)
        ADD_DEFINITIONS("-D__debug__")
	ADD_DEFINITIONS(
		"-Wparentheses -Wreturn-type -Wpointer-arith"
                "-Wwrite-strings -Wno-non-template-friend -Winline"
                "-Wformat -Wshadow -Woverloaded-virtual")
    ENDIF()
    IF(APPLE)
        ADD_DEFINITIONS("-D__mac__ -Dmac -Wno-reorder")
        FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
        FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
        SET(EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
    ELSE()
        ADD_DEFINITIONS("-Dlux")
    ENDIF()

    ADD_DEFINITIONS("'-DmDeclareRcsID=static const char* __attribute__ ((unused)) rcsID'")
        
ENDIF(UNIX)

IF(WIN32)
    ADD_DEFINITIONS("/W1 /Ob1 /Zc:wchar_t-")
    SET(EXTRA_LIBS "ws2_32" "shlwapi")
    ADD_DEFINITIONS( "\"-DmDeclareRcsID=static const char* rcsID\"")
ENDIF()

