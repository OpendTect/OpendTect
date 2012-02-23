#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODPlatformUtils.cmake,v 1.24 2012-02-23 15:30:06 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    IF( OD_DEBUG )
        ADD_DEFINITIONS("-D__debug__")
	ADD_DEFINITIONS(  "-ggdb3"
		"-Wparentheses -Wreturn-type -Wpointer-arith"
                "-Wwrite-strings -Wno-non-template-friend -Winline"
                "-Wformat -Wshadow -Woverloaded-virtual"
		"-Wno-char-subscripts -Wno-sign-compare" )
    ENDIF()
    IF(APPLE)
	SET(OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
        ADD_DEFINITIONS("-D__mac__ -Dmac -Wno-reorder -Wno-unused")
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

