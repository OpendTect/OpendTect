#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_ADD_BREAKPAD )
   if ( OD_ENABLE_BREAKPAD )
	set( BREAKPAD_DIR "" CACHE PATH "BREAKPAD Location (upto the 'src' directory)" )
	if( WIN32 )
	    find_library( BREAKPADLIB_DEBUG NAMES exception_handler 
		      PATHS ${BREAKPAD_DIR}/lib/Debug
		      REQUIRED )
	    find_library( BREAKPADLIB_RELEASE NAMES exception_handler 
		PATHS ${BREAKPAD_DIR}/lib/Release
		      REQUIRED )
	    find_library( BREAKPADCOMMONLIB_DEBUG NAMES common 
		PATHS ${BREAKPAD_DIR}/lib/Debug
		      REQUIRED )
	    find_library( BREAKPADCOMMONLIB_RELEASE NAMES common 
		PATHS ${BREAKPAD_DIR}/lib/Release
		      REQUIRED )
	    find_library( BREAKPADCLIENTLIB_DEBUG NAMES crash_generation_client 
		PATHS ${BREAKPAD_DIR}/lib/Debug
		      REQUIRED )
	    find_library( BREAKPADCLIENTLIB_RELEASE NAMES crash_generation_client 
		PATHS ${BREAKPAD_DIR}/lib/Release
		      REQUIRED )

	    set(BREAKPADMODULES LIB COMMONLIB CLIENTLIB )
	    foreach( MOD ${BREAKPADMODULES} )
		if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
		    if ( BREAKPAD${MOD}_RELEASE )
			list(APPEND OD_BREAKPADLIBS ${BREAKPAD${MOD}_RELEASE} )
		    elseif( BREAKPAD${MOD}_DEBUG )
			list(APPEND OD_BREAKPADLIBS ${BREAKPAD${MOD}_DEBUG} )
	            else()
			message( FATAL_ERROR "BREAKPAD${MOD} is missing" )
		    endif()
		elseif( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		    if ( BREAKPAD${MOD}_DEBUG )
			list(APPEND OD_BREAKPADLIBS ${BREAKPAD${MOD}_DEBUG} )
		    elseif( BREAKPAD${MOD}_RELEASE )
			list(APPEND OD_BREAKPADLIBS ${BREAKPAD${MOD}_RELEASE} )
	    	    else()
			message( FATAL_ERROR "BREAKPAD${MOD} is missing" )
		    endif()
		endif()
		unset( BREAKPAD${MOD}_DEBUG CACHE )
		unset( BREAKPAD${MOD}_RELEASE CACHE )
	    endforeach()
	elseif( APPLE )
	    #TODO
	else() # Linux
	    set(BREAKPADMODULES LIB CLIENTLIB )
	    find_library( BREAKPADLIB NAMES breakpad
		      PATHS ${BREAKPAD_DIR}/lib
		      REQUIRED )
	    find_library( BREAKPADCLIENTLIB NAMES breakpad_client
		      PATHS ${BREAKPAD_DIR}/lib
		      REQUIRED )
	    list(APPEND OD_BREAKPADLIBS ${BREAKPADLIBS} ${BREAKPADCLIENTLIB} )
	    unset( BREAKPADLIBS CACHE )
	    unset( BREAKPADCLIENTLIB CACHE )
	endif()

	find_program( BREAKPAD_DUMPSYMS_EXECUTABLE NAMES dump_syms 
		  PATHS ${BREAKPAD_DIR}/bin )

	if ( UNIX )
	    find_program( BREAKPAD_STACKWALK_EXECUTABLE NAMES minidump_stackwalk 
		  PATHS ${BREAKPAD_DIR}/bin )
	    OD_INSTALL_PROGRAM( "${BREAKPAD_STACKWALK_EXECUTABLE}" )
	endif( UNIX )
	
	install( DIRECTORY "${OD_BINARY_BASEDIR}/${OD_LIB_RELPATH_DEBUG}/symbols"
		 DESTINATION "${OD_LIB_INSTALL_PATH_DEBUG}"
		 CONFIGURATIONS Debug )
	install( DIRECTORY "${OD_BINARY_BASEDIR}/${OD_LIB_RELPATH_RELEASE}/symbols"
		 DESTINATION "${OD_LIB_INSTALL_PATH_RELEASE}"
		 CONFIGURATIONS Release )

   endif()
endmacro(OD_ADD_BREAKPAD)

macro( OD_SETUP_BREAKPAD )

    if( OD_ENABLE_BREAKPAD )

	list(APPEND OD_MODULE_INCLUDESYSPATH
		    "${BREAKPAD_DIR}/include/breakpad" )
	list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_BREAKPADLIBS} )

	add_definitions( -DHAS_BREAKPAD )

    endif(OD_ENABLE_BREAKPAD)

endmacro(OD_SETUP_BREAKPAD)

macro ( OD_GENERATE_SYMBOLS TRGT )
    if ( BREAKPAD_DUMPSYMS_EXECUTABLE )

	#Method to create timestamp file (and symbols). Will only trigger if out of date
	add_custom_command( TARGET ${TRGT} POST_BUILD 
		COMMAND ${CMAKE_COMMAND}
			-DLIBRARY=$<TARGET_FILE:${TRGT}>
			-DSYM_DUMP_EXECUTABLE=${BREAKPAD_DUMPSYMS_EXECUTABLE}
			-P ${OpendTect_DIR}/CMakeModules/GenerateSymbols.cmake
		DEPENDS ${TRGT}
		COMMENT "Generating symbols for ${TRGT}" )

    endif( BREAKPAD_DUMPSYMS_EXECUTABLE )
endmacro()
