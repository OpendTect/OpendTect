#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.15 2012-02-10 13:25:24 cvskris Exp $
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
	SET(OD_SUPPRESS_UNDEF_FLAGS "-flat_namespace -undefined suppress" )
    ELSE()
        ADD_DEFINITIONS("-Dlux")
    ENDIF()

    ADD_DEFINITIONS("'-DmDeclareRcsID=static const char* __attribute__ ((unused)) rcsID'")
    SET(OD_STATIC_EXTENSION ".a")
        
ENDIF(UNIX)

#Create Launchers on Windows and Mac OS
IF( DEFINED WIN32 OR (UNIX AND APPLE) )
    SET( OD_CREATE_LAUNCHERS 1 )
ENDIF()

IF(WIN32)
    SET(OD_EXTRA_COINFLAGS " /DCOIN_DLL /DSIMVOLEON_DLL /DSOQT_DLL" )
    ADD_DEFINITIONS("/W1 /Ob1 /MTd /vmg /Zc:wchar_t-")
    SET(EXTRA_LIBS "ws2_32" "shlwapi")
    ADD_DEFINITIONS( "\"-DmDeclareRcsID=static const char* rcsID\"")
    SET(OD_STATIC_EXTENSION ".lib")
ENDIF()

