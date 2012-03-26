#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.30 2012-03-26 10:31:29 cvsdgb Exp $
#_______________________________________________________________________________

#Discover 64 or 32 bits
IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET( OD_64BIT 1 )
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
        ADD_DEFINITIONS("-D__mac__ -Dmac")
        FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
        FIND_LIBRARY(STDCPP_LIBRARY stdc++ REQUIRED )
        SET(EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
	SET(OD_SUPPRESS_UNDEF_FLAGS "-flat_namespace -undefined suppress" )

	#NEEDED AS LONG AS WE HAVE COIN
	SET(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
	SET( OD_PLFSUBDIR mac )
    ELSE() #Linux
	#SET(OD_LIB_LINKER_NEEDS_ALL_LIBS 1) #Not needed, but percentages are screwed otherwise.

	IF ( OD_64BIT )
	    SET ( OD_PLFSUBDIR "lux64" )
	    ADD_DEFINITIONS("-Dlux64")
	ELSE()
	    SET ( OD_PLFSUBDIR "lux32" )
	    ADD_DEFINITIONS("-Dlux32")
	ENDIF()
        ADD_DEFINITIONS("-Dlux")
    ENDIF()

    ADD_DEFINITIONS("'-DmDeclareRcsID=static const char* __attribute__ ((unused)) rcsID'")
    SET(OD_STATIC_EXTENSION ".a")
    IF( OD_DEBUG )
        ADD_DEFINITIONS("-D__debug__")
	ADD_DEFINITIONS(  "-ggdb3"
		"-Wparentheses -Wreturn-type -Wpointer-arith"
                "-Wwrite-strings -Winline"
                "-Wformat -Wshadow "
		"-Wno-char-subscripts -Wno-sign-compare" )
	IF ( CMAKE_CXX_FLAGS STREQUAL "" )
	    SET( CMAKE_CXX_FLAGS
		"-Wno-non-template-friend  -Woverloaded-virtual -Wno-reorder -Wno-unused"
		 CACHE STRING "CC flags" FORCE )
	ENDIF()
    ELSE( OD_DEBUG )
	ADD_DEFINITIONS( -Wno-inline )
    ENDIF()

        
ENDIF(UNIX)

#Create Launchers on Windows and Mac OS
IF( DEFINED WIN32 OR (UNIX AND APPLE) )
    SET( OD_CREATE_LAUNCHERS 1 )
ENDIF()

IF(WIN32)
    SET(OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
    SET ( OD_PLATFORM_LINK_OPTIONS "/LARGEADDRESSAWARE" )
    SET(OD_EXTRA_COINFLAGS " /DCOIN_DLL /DSIMVOLEON_DLL /DSOQT_DLL" )
    ADD_DEFINITIONS("/W1 /Ob1 /MDd /vmg /Zc:wchar_t")
    SET(EXTRA_LIBS "ws2_32" "shlwapi")
    ADD_DEFINITIONS( "\"-DmDeclareRcsID=static const char* rcsID\"")
    SET(OD_STATIC_EXTENSION ".lib")
    IF ( OD_64BIT )
        SET ( OD_PLFSUBDIR "win64" )
    ELSE()
        SET ( OD_PLFSUBDIR "win32" )
    ENDIF()
ENDIF()

