#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_BREAKPAD_TARGET TRGT )

    if ( NOT IS_DIRECTORY "${BREAKPAD_DIR}/include/breakpad" )
	message( SEND_ERROR "Cannot find breakpad header files at ${BREAKPAD_DIR}/include/breakpad" )
    elseif ( (BREAKPAD${MOD}_RELEASE AND EXISTS "${BREAKPAD${MOD}_RELEASE}") OR
	     (BREAKPAD${MOD}_DEBUG AND EXISTS "${BREAKPAD${MOD}_DEBUG}") )
	add_library( breakpad::${TRGT} STATIC IMPORTED GLOBAL )
	foreach( config RELEASE;DEBUG )
	    if ( BREAKPAD${MOD}_${config} AND EXISTS "${BREAKPAD${MOD}_${config}}" )
		list( APPEND BREAKPAD_CONFIGS ${config} )
		set_target_properties( breakpad::${TRGT} PROPERTIES
			IMPORTED_LOCATION_${config} "${BREAKPAD${MOD}_${config}}" )
		unset( BREAKPAD_LOCATION_${config} )
	    endif()
	    unset( BREAKPAD${MOD}_${config} CACHE )
	endforeach()
	set_target_properties( breakpad::${TRGT} PROPERTIES
		IMPORTED_CONFIGURATIONS "${BREAKPAD_CONFIGS}"
		INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_DIR}/include/breakpad" )
	od_map_configurations( breakpad::${TRGT} )
	unset( BREAKPAD_CONFIGS )
    endif()

endmacro(OD_SETUP_BREAKPAD_TARGET)

macro ( OD_GENERATE_SYMBOLS TRGT )
    if ( OD_ENABLE_BREAKPAD AND EXISTS "${BREAKPAD_DUMPSYMS_EXECUTABLE}" )
	#Method to create timestamp file (and symbols). Will only trigger if out of date
	add_custom_command( TARGET ${TRGT} POST_BUILD 
		COMMAND ${CMAKE_COMMAND}
			-DLIBRARY=$<TARGET_FILE:${TRGT}>
			-DSYM_DUMP_EXECUTABLE=${BREAKPAD_DUMPSYMS_EXECUTABLE}
			-P ${OpendTect_DIR}/CMakeModules/GenerateSymbols.cmake
		DEPENDS ${TRGT}
		COMMENT "Generating symbols for ${TRGT}" )
    endif()
endmacro(OD_GENERATE_SYMBOLS)

macro( OD_ADD_BREAKPAD )

   if ( OD_ENABLE_BREAKPAD )
	set( BREAKPAD_DIR "" CACHE PATH "BREAKPAD Location (upto the 'src' directory)" )
	set( BREAKPADMODULES COMMON CLIENT )
	set( BREAKPADCOMPS breakpad client )
	if( WIN32 )
	    list( APPEND HANDLER )
	    list( APPEND BREAKPADCOMPS handler )
	    set( BREAKPADNAMES common crash_generation_client exception_handler )
	else()
	    set( BREAKPADNAMES libbreakpad.a libbreakpad_client.a )
	endif()

	foreach( MOD LIBNAME TRGT IN ZIP_LISTS BREAKPADMODULES BREAKPADNAMES BREAKPADCOMPS )
	    find_library( BREAKPAD${MOD}_RELEASE NAMES ${LIBNAME}
			  PATHS "${BREAKPAD_DIR}"
			  PATH_SUFFIXES lib lib/Release )
	    find_library( BREAKPAD${MOD}_DEBUG NAMES ${LIBNAME}
			  PATHS "${BREAKPAD_DIR}"
			  PATH_SUFFIXES lib/Debug )
	    OD_SETUP_BREAKPAD_TARGET( ${TRGT} )
	endforeach()

	find_program( BREAKPAD_DUMPSYMS_EXECUTABLE NAMES dump_syms
		      PATHS ${BREAKPAD_DIR}/bin )

	install( DIRECTORY "${OD_BINARY_BASEDIR}/${OD_RUNTIME_DIRECTORY}/symbols"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )

   endif(OD_ENABLE_BREAKPAD)

endmacro(OD_ADD_BREAKPAD)

macro( OD_SETUP_BREAKPAD )

    if ( OD_ENABLE_BREAKPAD )
	foreach( COMP ${BREAKPADCOMPS} )
	    if ( TARGET breakpad::${COMP} )
		list( APPEND OD_MODULE_EXTERNAL_LIBS breakpad::${COMP} )
	    else()
		message( SEND_ERROR "Cannot link against breakpad::${COMP}" )
	    endif()
	endforeach()

	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "HAS_BREAKPAD" )
    endif(OD_ENABLE_BREAKPAD)

endmacro(OD_SETUP_BREAKPAD)
