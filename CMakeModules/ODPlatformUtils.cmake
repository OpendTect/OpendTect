#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.18 2012-02-15 10:02:41 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    IF( OD_DEBUG MATCHES Debug)
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

	#NEEDED AS LONG AS WE HAVE COIN
	SET(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
	SET( OD_PLFSUBDIR mac )
    ELSE()
	IF ( OD_64BIT )
	    SET ( OD_PLFSUBDIR "lux64" )
	ELSE()
	    SET ( OD_PLFSUBDIR "lux32" )
	ENDIF()
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
    SET( OD_PLFSUBDIR "win" )
ENDIF()

