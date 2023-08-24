#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_DEBUG_FLAGS_GLOBAL )

    foreach( LANG C CXX )
	if ( CMAKE_${LANG}_COMPILER_ID STREQUAL "MSVC" )
	    foreach( config DEBUG;RELWITHDEBINFO )
		if ( "${CMAKE_${LANG}_FLAGS_${config}}" MATCHES "/Zi" )
		    string( REPLACE "/Zi" "/Z7" CMAKE_${LANG}_FLAGS_${config} "${CMAKE_${LANG}_FLAGS_${config}}" )
		    set( CMAKE_${LANG}_FLAGS_${config} "${CMAKE_${LANG}_FLAGS_${config}}" CACHE STRING "Flags used by the ${LANG} compiler during ${config} builds." FORCE )
		endif()
	    endforeach()
	elseif ( CMAKE_${LANG}_COMPILER_ID STREQUAL "GNU" )
	    foreach( config DEBUG;RELWITHDEBINFO )
		if ( "${CMAKE_${LANG}_FLAGS_${config}}" MATCHES "-g" AND NOT "${CMAKE_${LANG}_FLAGS_${config}}" MATCHES "-ggdb3" )
		    string( REPLACE "-g" "-ggdb3" CMAKE_${LANG}_FLAGS_${config} "${CMAKE_${LANG}_FLAGS_${config}}" )
		    set( CMAKE_${LANG}_FLAGS_${config} "${CMAKE_${LANG}_FLAGS_${config}}" CACHE STRING "Flags used by the ${LANG} compiler during ${config} builds." FORCE )
		endif()
	    endforeach()
	endif()
    endforeach()

endmacro(OD_SETUP_DEBUG_FLAGS_GLOBAL)


if ( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    message( FATAL_ERROR "32-bit platforms are no longer supported" )
endif()

if( UNIX )

    if( APPLE )

	set ( SHLIB_EXTENSION dylib )
	if ( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64" )
	    set ( OD_PLFSUBDIR macarm )
	else()
	    set ( OD_PLFSUBDIR macintel )
	endif()

	option( AVOID_CLANG_ERROR "Avoid CLang error" OFF )
	if ( AVOID_CLANG_ERROR )
	    set ( CMAKE_CXX_FLAGS
		  "${CMAKE_CXX_FLAGS} -D__MAC_LLVM_COMPILER_ERROR__" )
	endif()
	if ( CMAKE_GENERATOR STREQUAL "Xcode" )
	    set( OD_SUPPRESS_WARNINGS_NOT_ON_WINDOWS yes )
	endif()

	#For some versions of XCode
	set ( CMAKE_FIND_LIBRARY_PREFIXES lib )
	set ( CMAKE_FIND_LIBRARY_SUFFIXES .dylib )

        find_library( APP_SERVICES_LIBRARY ApplicationServices
		      PATH ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks )
	set ( EXTRA_LIBS ${APP_SERVICES_LIBRARY} )
	set ( OD_SUPPRESS_UNDEF_FLAGS "-flat_namespace -undefined suppress" )

    else() # Not Apple

	set ( SHLIB_EXTENSION so )
	set ( OD_PLFSUBDIR "lux64" )
	set ( OD_EXECUTABLE_COMPILE_FLAGS "-fPIC" )

	if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )

	    if ( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.2 )
		set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wignored-qualifiers" )
	    endif()

	    #Compile time optimization
	    if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
		set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=nocona" )
		if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.3)
		    set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -ftree-vectorize" )
		endif()
	    else()
		set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=core-avx2" )
	    endif()

	endif()

	#Make all targets look for dependent libraries in the same location as they are in
	set( CMAKE_INSTALL_RPATH "\$ORIGIN")

    endif()

    set ( OD_LINESEP "\n" )
    set ( CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")
    set ( CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ")
    add_definitions("'-DmUnusedVar=__attribute__ ((unused))'")
    add_definitions("'-DmUsedVar=__attribute__ ((used))'")
    set (OD_STATIC_EXTENSION ".a")
    if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )

	OD_SETUP_DEBUG_FLAGS_GLOBAL()
	if ( NOT DEFINED OD_SUPPRESS_WARNINGS_NOT_ON_WINDOWS )
	    set ( CMAKE_CXX_FLAGS "-Woverloaded-virtual -Wshadow -Wunused ${CMAKE_CXX_FLAGS}" )
	endif()
	set ( CMAKE_CXX_FLAGS "-Wno-reorder -Wmissing-braces -Wparentheses -Wsequence-point ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wswitch -Wunused-function ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wwrite-strings -Wpointer-arith -Winline ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wformat -Wmissing-field-initializers ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wreturn-type -Winit-self -Wno-char-subscripts ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wstrict-aliasing ${CMAKE_CXX_FLAGS}" )

	#use below and you'll be flooded with warnings:
	#set ( CMAKE_CXX_FLAGS "-Wno-sign-compare -Wcast-align -Wconversion ${CMAKE_CXX_FLAGS}" )
	set ( CMAKE_CXX_FLAGS "-Wno-sign-compare -Wcast-align ${CMAKE_CXX_FLAGS}" )

	foreach( _config MINSIZEREL;RELWITHDEBINFO;RELEASE )
	    set ( CMAKE_CXX_FLAGS_${_config} "-Wno-inline -Wuninitialized -Winit-self ${CMAKE_CXX_FLAGS_${_config}}" )
	endforeach()
	unset( _config )
	set ( CMAKE_C_FLAGS "-Wmissing-declarations -Wunused -Wimplicit-int ${CMAKE_C_FLAGS}" )
	set ( CMAKE_C_FLAGS "-Wimplicit-function-declaration -Wpointer-sign ${CMAKE_C_FLAGS}" )
	set ( CMAKE_C_FLAGS "-Wstrict-aliasing -Wstrict-prototypes ${CMAKE_C_FLAGS}" )

    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Intel" OR
	    CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM" )

	set ( CMAKE_SKIP_RPATH TRUE )
	set ( EXTRA_LIBS "imf" "m" ) #avoid bogus warning: https://wiki.hpcc.msu.edu/display/Issues/feupdateenv+is+not+implemented+and+will+always+fail

    endif ()

else() # windows

    set ( SHLIB_EXTENSION dll )
    set ( OD_PLFSUBDIR "win64" )

    #Create Launchers on Windows
    set ( OD_CREATE_LAUNCHERS TRUE )

    #Setting Stack Reserve size for Executables only
    if ( NOT DEFINED STACK_RESERVE_SIZE )
        MATH ( EXPR STACK_RESERVE_SIZE "8 * 1024 * 1024" ) #Setting default stack size to 8MB
	set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:${STACK_RESERVE_SIZE}" )
    else()
	set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:${STACK_RESERVE_SIZE}" )
    endif()
    set( STACK_RESERVE_SIZE ${STACK_RESERVE_SIZE} CACHE STRING "Stack Reserve Size" )

    OD_SETUP_DEBUG_FLAGS_GLOBAL()
    if ( CMAKE_GENERATOR MATCHES "Visual Studio" )
	set ( CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}" )
    endif()
    set ( CMAKE_CXX_FLAGS "/vmg /EHsc ${CMAKE_CXX_FLAGS}")
    set ( EXTRA_LIBS "ws2_32" "shlwapi")
    set ( CMAKE_CXX_FLAGS "-DmUnusedVar= ${CMAKE_CXX_FLAGS}")
    set ( CMAKE_CXX_FLAGS "-DmUsedVar= ${CMAKE_CXX_FLAGS}")
    set ( CMAKE_C_FLAGS "-DmUnusedVar= ${CMAKE_C_FLAGS}")
    set ( CMAKE_C_FLAGS "-DmUsedVar= ${CMAKE_C_FLAGS}")
    string ( REPLACE "/W3 " "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
    set ( CMAKE_CXX_FLAGS " /W4 ${CMAKE_CXX_FLAGS}" )

    set ( CMAKE_CXX_FLAGS "/wd4389 ${CMAKE_CXX_FLAGS}" ) # unsigned/signed mismatch
    set ( CMAKE_CXX_FLAGS "/wd4458 ${CMAKE_CXX_FLAGS}" ) # warnings for Microsoft's GDI+ headers
    set ( CMAKE_CXX_FLAGS "/wd4018 ${CMAKE_CXX_FLAGS}" ) # unsigned/signed compare
    set ( CMAKE_CXX_FLAGS "/wd4505 ${CMAKE_CXX_FLAGS}" ) # unreferenced local function removed
    set ( CMAKE_CXX_FLAGS "/wd4121 ${CMAKE_CXX_FLAGS}" ) # Alignmnent of variables
    set ( CMAKE_CXX_FLAGS "/wd4250 ${CMAKE_CXX_FLAGS}" ) # Diamond inheritance problems
    set ( CMAKE_CXX_FLAGS "/wd4355 ${CMAKE_CXX_FLAGS}" ) # The this pointer is valid only within nonstatic member functions.
    set ( CMAKE_CXX_FLAGS "/wd4100 ${CMAKE_CXX_FLAGS}" ) # unreferenced formal parameter
    #set ( CMAKE_CXX_FLAGS " /wd4701 ${CMAKE_CXX_FLAGS}" ) # local variable used without being initialized
    set ( CMAKE_CXX_FLAGS "/wd4800 ${CMAKE_CXX_FLAGS}" ) # forcing value to bool 'true' or 'false' (performance warning)
    set ( CMAKE_CXX_FLAGS "/wd4251 ${CMAKE_CXX_FLAGS}" ) # 'identifier' : dll-interface
    set ( CMAKE_CXX_FLAGS "/wd4275 ${CMAKE_CXX_FLAGS}" ) # 'identifier' : dll-interface
    set ( CMAKE_CXX_FLAGS "/wd4273 ${CMAKE_CXX_FLAGS}" ) # inconsistent dll linkage
    set ( CMAKE_CXX_FLAGS "/wd4101 ${CMAKE_CXX_FLAGS}" ) # The local variable is never used (disable only for Windows)
    set ( CMAKE_CXX_FLAGS "/wd4267 ${CMAKE_CXX_FLAGS}" ) # conversion from 'size_t' to 'type', possible loss of data
    set ( CMAKE_CXX_FLAGS "/wd4267 ${CMAKE_CXX_FLAGS}" ) # conversion from 'size_t' to 'type', possible loss of data
    set ( CMAKE_CXX_FLAGS "/wd4512 ${CMAKE_CXX_FLAGS}" ) # class' : assignment operator could not be generated (not important)
    set ( CMAKE_CXX_FLAGS "/wd4127 ${CMAKE_CXX_FLAGS}" ) # conditional expression is constant, e.g. while ( true )
    set ( CMAKE_CXX_FLAGS "/wd4189 ${CMAKE_CXX_FLAGS}" ) # local variable is initialized but not referenced
    set ( CMAKE_CXX_FLAGS "/wd4305 ${CMAKE_CXX_FLAGS}" ) # truncation from dowble to float
    if ( MSVC_VERSION VERSION_LESS 1900 ) #Adding these flags if VS version is less than 12
	set ( CMAKE_CXX_FLAGS_DEBUG "/WX ${CMAKE_CXX_FLAGS_DEBUG}" ) # Treat warnings as errors
    endif()
    if ( MSVC_VERSION VERSION_GREATER 1800 ) #Adding this flag if VS version is greater than 12 on win64 platform
	set ( CMAKE_CXX_FLAGS "/wd4244 ${CMAKE_CXX_FLAGS}" ) # conversion' conversion from 'type1' to 'type2', possible loss of data ( _int64 to int )
    endif()
    set ( CMAKE_CXX_FLAGS "/wd4714 ${CMAKE_CXX_FLAGS}" ) # _forceinline function not inlined
    set ( CMAKE_CXX_FLAGS "/wd4589 ${CMAKE_CXX_FLAGS}" ) # ignore initializer for abstract base classes
    if ( MSVC_VERSION VERSION_GREATER 1923 ) #Adding this flag if VS version is greater than 17 on win64 platform
	set ( CMAKE_CXX_FLAGS "/wd4723 ${CMAKE_CXX_FLAGS}" ) # potential divide by 0
    endif()
    if ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
	list ( APPEND OD_PLATFORM_COMPILE_OPTIONS "/Zc:__cplusplus" )
    endif()

    set ( OD_STATIC_EXTENSION ".lib" )
    set ( OD_EXECUTABLE_EXTENSION ".exe" )

    set ( OD_GUI_SYSTEM "WIN32" )
    set ( OD_LINESEP "\n" ) #Will be converted to \r\n when written to files by cmake
    set ( OD_UAC_LINKFLAGS "/level='requireAdministrator'" "/uiAccess='false'" )

endif( UNIX )

macro( OD_SETUP_DEBUGFLAGS )

    if ( OD_ENABLE_BREAKPAD AND EXISTS "${BREAKPAD_DUMPSYMS_EXECUTABLE}" )
	if ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
	    list ( APPEND OD_MODULE_COMPILE_OPTIONS "/Z7" )
	    list ( APPEND OD_MODULE_LINK_OPTIONS "/debug" )
	elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
	    list ( APPEND OD_MODULE_COMPILE_OPTIONS "-ggdb3" )
	endif()
    endif()

endmacro(OD_SETUP_DEBUGFLAGS)

macro( OD_INSTALL_PDB_FILES )
    if ( OD_ENABLE_BREAKPAD AND EXISTS "${BREAKPAD_DUMPSYMS_EXECUTABLE}" )
	install( FILES "$<TARGET_PDB_FILE:${OD_MODULE_NAME}>"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    else()
	install( FILES "$<TARGET_PDB_FILE:${OD_MODULE_NAME}>"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}"
		 CONFIGURATIONS Debug;RelWithDebInfo )
    endif()
endmacro(OD_INSTALL_PDB_FILES)
