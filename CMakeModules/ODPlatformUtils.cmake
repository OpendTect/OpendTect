#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

#Discover 64 or 32 bits
set ( OD_64BIT 1 )
if ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set ( OD_64BIT )
endif()

#Discover Debug
if( ${CMAKE_BUILD_TYPE} MATCHES Release)
    set ( OD_DEBUG )
else()
    set ( OD_DEBUG 1 )
endif()

set ( SET_SYMBOLS -D__hassymbols__ )
set ( SET_DEBUG -D__debug__ )

set ( CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SET_SYMBOLS} ")
set ( CMAKE_C_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SET_SYMBOLS} ")

if(UNIX) #Apple an Linux

    if(APPLE)
	set ( OD_GCC_COMPILER 1 )
	if ( ${CMAKE_GENERATOR} STREQUAL "Xcode" )
	    set ( OD_EXTRA_OSGFLAGS "-Wno-shadow -Wno-overloaded-virtual" ) #Sysroot does not do the job
	    set ( OD_EXTRA_COINFLAGS "-Wno-shadow -Wno-overloaded-virtual" ) #Sysroot does not do the job
	endif()

	set ( OD_SET_TARGET_PROPERTIES 1 )

	#For some versions of XCode
	set ( CMAKE_FIND_LIBRARY_PREFIXES lib )
	set ( CMAKE_FIND_LIBRARY_SUFFIXES .dylib )

	set ( OD_LIB_LINKER_NEEDS_ALL_LIBS 1 )
	set ( OD_PLATFORM_LINK_OPTIONS "-arch x86_64" )
        add_definitions("-arch x86_64")
        find_library( APP_SERVICES_LIBRARY ApplicationServices
		      PATH ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks )
        find_library( STDCPP_LIBRARY stdc++ REQUIRED )
        set (EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
	set (OD_SUPPRESS_UNDEF_FLAGS "-flat_namespace -undefined suppress" )
	#set ( OD_GUI_SYSTEM "MACOSX_BUNDLE" )

	set ( OD_PLFSUBDIR mac )

	#NEEDED AS LONG AS WE HAVE COIN
	set ( CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
    else() #Linux
	#Not on most platforms, but for the few that does, it's better
	set (OD_LIB_LINKER_NEEDS_ALL_LIBS 1)

	if ( OD_64BIT )
	    set ( OD_PLFSUBDIR "lux64" )
	else()
	    set ( OD_PLFSUBDIR "lux32" )
	    add_definitions("-march=pentium4")
	endif()

	if ( CMAKE_COMPILER_IS_GNUCC  ) 
	    set ( OD_GCC_COMPILER 1 )
	    execute_process( COMMAND ${CMAKE_C_COMPILER} -dumpversion
			     OUTPUT_VARIABLE GCC_VERSION )

	    if ( GCC_VERSION VERSION_GREATER 4.2 )
		set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wignored-qualifiers" )
	    endif()

	    if ( GCC_VERSION VERSION_LESS 4.2 )
		message( "Turning down gcc optimization to -O2" )
		set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2" )
	    endif()

	    set ( CMAKE_CXX_FLAGS "-Wno-non-template-friend ${CMAKE_CXX_FLAGS}" )

	    if ( (CMAKE_CXX_COMPILER STREQUAL "/usr/bin/g++4") OR
		 (CMAKE_C_COMPILER STREQUAL "/usr/bin/gcc4") )
		set( CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2" )
		set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2" )
	    endif()
	endif(CMAKE_COMPILER_IS_GNUCC)

    endif()

    set ( OD_LINESEP "\n" )
    add_definitions("'-DmUnusedVar=__attribute__ ((unused))'")
    add_definitions("'-DmUsedVar=__attribute__ ((used))'")
    set (OD_STATIC_EXTENSION ".a")
    if ( OD_GCC_COMPILER )
	set ( CMAKE_CXX_FLAGS "-Woverloaded-virtual -Wno-reorder ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wunused -Wmissing-braces -Wparentheses -Wsequence-point ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wswitch -Wunused-function -Wunused-label ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wshadow -Wwrite-strings -Wpointer-arith -Winline ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wformat -Wmissing-field-initializers ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wreturn-type -Winit-self -Wno-char-subscripts ${CMAKE_CXX_FLAGS}" )

	#use below and you'll be flooded with warnings:
	#set ( CMAKE_CXX_FLAGS "-Wno-sign-compare -Wcast-align -Wconversion ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wno-sign-compare -Wcast-align ${CMAKE_CXX_FLAGS}" )


	set ( CMAKE_CXX_FLAGS_RELEASE "-Wno-inline ${CMAKE_CXX_FLAGS_RELEASE}" )

	set ( CMAKE_C_FLAGS "-Wmissing-declarations -Wunused -Wimplicit-int ${CMAKE_C_FLAGS}" )
	set ( CMAKE_C_FLAGS "-Wimplicit-function-declaration -Wpointer-sign -Wstrict-prototypes ${CMAKE_C_FLAGS}" )

	set ( CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${SET_SYMBOLS} ${SET_DEBUG} -ggdb3" )
	set ( CMAKE_C_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${SET_SYMBOLS} ${SET_DEBUG} -ggdb3" )

    else() # Intel compiler
	set ( CMAKE_SKIP_RPATH TRUE )
    endif( OD_GCC_COMPILER )

endif(UNIX)

if(WIN32)
    #Create Launchers on Windows
    set ( OD_CREATE_LAUNCHERS 1 )
    set ( OD_SET_TARGET_PROPERTIES 1 )

    set (OD_LIB_LINKER_NEEDS_ALL_LIBS 1)
    set  ( OD_PLATFORM_LINK_OPTIONS "/LARGEADDRESSAWARE /debug" ) #/debug will enable the generation of pdb-files.
    
    set (OD_EXTRA_COINFLAGS " /DCOIN_DLL /DSIMVOLEON_DLL /DSOQT_DLL /wd4244" )
    add_definitions("/Ob1 /vmg /Zc:wchar_t- /EHsc")
    #add_definitions("/MP")
    set (EXTRA_LIBS "ws2_32" "shlwapi")
    add_definitions(  "\"-DmUnusedVar=\"")
    add_definitions(  "\"-DmUsedVar=\"")
    add_definitions( /W4 )
    
    add_definitions( /wd4389 ) # unsigned/signed mismatch
    add_definitions( /wd4018 ) # unsigned/signed compare
    add_definitions( /wd4505 ) # unreferenced local function removed
    add_definitions( /wd4121 ) # Alignmnent of variables
    add_definitions( /wd4250 ) # Diamond inheritance problems
    add_definitions( /wd4355 ) # The this pointer is valid only within nonstatic member functions.
    add_definitions( /wd4100 ) # unreferenced formal parameter
    #add_definitions( /wd4701 ) # local variable used without being initialized
    add_definitions( /wd4800 ) # forcing value to bool 'true' or 'false' (performance warning)
    add_definitions( /wd4251 ) # 'identifier' : dll-interface
    add_definitions( /wd4275 ) # 'identifier' : dll-interface
    add_definitions( /wd4273 ) # inconsistent dll linkage
    add_definitions( /wd4996 ) # function': was declared deprecated
    add_definitions( /wd4101 ) # The local variable is never used (disable only for Windows)
    add_definitions( /wd4267 ) # conversion from 'size_t' to 'type', possible loss of data
    add_definitions( /wd4267 ) # conversion from 'size_t' to 'type', possible loss of data
    add_definitions( /wd4512 ) # class' : assignment operator could not be generated (not important)
    add_definitions( /wd4127 ) # conditional expression is constant, e.g. while ( true )
    add_definitions( /wd4189 ) # local variable is initialized but not referenced
    add_definitions( /wd4305 ) # truncation from dowble to float

    set (OD_STATIC_EXTENSION ".lib")
    set (OD_EXECUTABLE_EXTENSION ".exe" )
    if ( OD_64BIT )
        set  ( OD_PLFSUBDIR "win64" )
    else()
        set  ( OD_PLFSUBDIR "win32" )
	add_definitions( /wd4244 ) # conversion' conversion from 'type1' to 'type2', possible loss of data ( _int64 to int ) 
    endif()

    set  ( OD_GUI_SYSTEM "WIN32" )
    set ( OD_LINESEP "\n" ) #Will be converted to \r\n when written to files by cmake
endif()

add_definitions( "\"-D__${OD_PLFSUBDIR}__=1\"" )

