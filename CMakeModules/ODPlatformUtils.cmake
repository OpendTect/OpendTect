#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.52 2012-07-18 09:09:05 cvskris Exp $
#_______________________________________________________________________________

#Discover 64 or 32 bits
set ( OD_64BIT 1 )
IF ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set ( OD_64BIT )
ENDIF()

#Discover Debug
IF( ${CMAKE_BUILD_TYPE} MATCHES Release)
    set ( OD_DEBUG )
ELSE()
    set ( OD_DEBUG 1 )
ENDIF()

IF(UNIX) #Apple an Linux
    IF(APPLE)
	set (OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
	set ( OD_PLATFORM_LINK_OPTIONS "-arch x86_64" )
        ADD_DEFINITIONS("-arch x86_64")
        FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
        FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
        set (EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
	set (OD_SUPPRESS_UNDEF_FLAGS "-flat_namespace -undefined suppress" )
	IF(!OD_DEBUG)
	    set  ( OD_GUI_SYSTEM "MACOSX_BUNDLE" )
	ENDIF()

	set ( OD_PLFSUBDIR mac )

	#NEEDED AS LONG AS WE HAVE COIN
	set (CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
    ELSE() #Linux

	#Not on most platforms, but for the few that does, it's better
	set (OD_LIB_LINKER_NEEDS_ALL_LIBS 1)

	IF ( OD_64BIT )
	    set  ( OD_PLFSUBDIR "lux64" )
	ELSE()
	    set  ( OD_PLFSUBDIR "lux32" )
	    ADD_DEFINITIONS("-march=pentium4")
	ENDIF()
    ENDIF()

    ADD_DEFINITIONS("'-DmUnusedVar=__attribute__ ((unused))'")
    set (OD_STATIC_EXTENSION ".a")
    IF( OD_DEBUG )
        ADD_DEFINITIONS("-D__debug__")
	ADD_DEFINITIONS(  "-ggdb3" )
    ENDIF()

    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-non-template-friend" )
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wno-reorder" )
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused -Wmissing-braces -Wparentheses -Wsequence-point" )
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch -Wunused-function -Wunused-label" )
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -Wwrite-strings -Wpointer-arith -Winline" )
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wmissing-field-initializers" )

    set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-inline -Wuninitialized -Winit-self" )

    set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wunused" )

#Extra:
# -Wempty-body
# -Waddress
# -Wunreachable code

        
ENDIF(UNIX)

#Create Launchers on Windows
IF( DEFINED WIN32 )
    set ( OD_CREATE_LAUNCHERS 1 )
ENDIF()

IF(WIN32)
    set (OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
    set  ( OD_PLATFORM_LINK_OPTIONS "/LARGEADDRESSAWARE" )
    set (OD_EXTRA_COINFLAGS " /DCOIN_DLL /DSIMVOLEON_DLL /DSOQT_DLL" )
    ADD_DEFINITIONS("/W1 /Ob1 /vmg /Zc:wchar_t-")
    set (EXTRA_LIBS "ws2_32" "shlwapi")
    ADD_DEFINITIONS(  "\"-DmUnusedVar=\"")
    set (OD_STATIC_EXTENSION ".lib")
    set (OD_EXECUTABLE_EXTENSION ".exe" )
    IF ( OD_64BIT )
        set  ( OD_PLFSUBDIR "win64" )
    ELSE()
        set  ( OD_PLFSUBDIR "win32" )
    ENDIF()

    set  ( OD_GUI_SYSTEM "WIN32" )
ENDIF()

ADD_DEFINITIONS( "\"-D__${OD_PLFSUBDIR}__=1\"" )

