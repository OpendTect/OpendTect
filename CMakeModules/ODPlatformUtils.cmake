#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.48 2012-07-17 06:07:01 cvskris Exp $
#_______________________________________________________________________________

#Discover 64 or 32 bits
SET( OD_64BIT 1 )
IF ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    SET( OD_64BIT )
ENDIF()

#Discover Debug
IF( ${CMAKE_BUILD_TYPE} MATCHES Release)
    SET( OD_DEBUG )
ELSE()
    SET( OD_DEBUG 1 )
ENDIF()

IF(UNIX) #Apple an Linux
    IF(APPLE)
	SET(OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
	SET( OD_PLATFORM_LINK_OPTIONS "-arch x86_64" )
        ADD_DEFINITIONS("-arch x86_64 -D__mac__ -Dmac")
        FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
        FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
        SET(EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
	SET(OD_SUPPRESS_UNDEF_FLAGS "-flat_namespace -undefined suppress" )
	IF(!OD_DEBUG)
	    SET ( OD_GUI_SYSTEM "MACOSX_BUNDLE" )
	ENDIF()

	#NEEDED AS LONG AS WE HAVE COIN
	SET(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
	SET( OD_PLFSUBDIR mac )
    ELSE() #Linux

	#Not on most platforms, but for the few that does, it's better
	SET(OD_LIB_LINKER_NEEDS_ALL_LIBS 1)

	IF ( OD_64BIT )
	    SET ( OD_PLFSUBDIR "lux64" )
	    ADD_DEFINITIONS("-Dlux64")
	ELSE()
	    SET ( OD_PLFSUBDIR "lux32" )
	    ADD_DEFINITIONS("-Dlux32 -march=pentium4")
	ENDIF()
        ADD_DEFINITIONS("-Dlux")
	
    ENDIF()

    ADD_DEFINITIONS("'-DmUnusedVar=__attribute__ ((unused))'")
    SET(OD_STATIC_EXTENSION ".a")
    IF( OD_DEBUG )
        ADD_DEFINITIONS("-D__debug__")
	ADD_DEFINITIONS(  "-ggdb3"
		"-Wparentheses -Wreturn-type -Wpointer-arith"
                "-Wwrite-strings -Winline"
                "-Wformat -Wshadow -Wswitch"
		"-Wno-char-subscripts -Wno-sign-compare" )
	IF ( CMAKE_CXX_FLAGS STREQUAL "" )
	    SET( CMAKE_CXX_FLAGS
		"-Wno-non-template-friend  -Woverloaded-virtual -Wno-reorder" )
	ENDIF()
	IF ( CMAKE_C_FLAGS STREQUAL "" )
	    SET( CMAKE_C_FLAGS "-Wmissing-declarations" )
	ENDIF()
    ELSE( OD_DEBUG )
	ADD_DEFINITIONS( -Wno-inline )
    ENDIF()

        
ENDIF(UNIX)

#Create Launchers on Windows and Mac OS
IF( DEFINED WIN32 )
    SET( OD_CREATE_LAUNCHERS 1 )
ENDIF()

IF(WIN32)
    SET(OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
    SET ( OD_PLATFORM_LINK_OPTIONS "/LARGEADDRESSAWARE" )
    SET(OD_EXTRA_COINFLAGS " /DCOIN_DLL /DSIMVOLEON_DLL /DSOQT_DLL" )
    ADD_DEFINITIONS("/W1 /Ob1 /vmg /Zc:wchar_t-")
    SET(EXTRA_LIBS "ws2_32" "shlwapi")
    ADD_DEFINITIONS(  "\"-DmUnusedVar=\"")
    SET(OD_STATIC_EXTENSION ".lib")
    SET(OD_EXECUTABLE_EXTENSION ".exe" )
    IF ( OD_64BIT )
        SET ( OD_PLFSUBDIR "win64" )
    ELSE()
        SET ( OD_PLFSUBDIR "win32" )
    ENDIF()

    SET ( OD_GUI_SYSTEM "WIN32" )
ENDIF()

