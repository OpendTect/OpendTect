#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Aneesh
#	RCS :		$Id$
#_______________________________________________________________________________


set( BREAKPAD_DIR "" CACHE PATH "BREAKPAD Location (upto the 'src' directory)" )
option ( OD_ENABLE_BREAKPAD "Use breakpad" )

if(OD_ENABLE_BREAKPAD)
    add_definitions( -DHAS_BREAKPAD )
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
	 OD_MERGE_LIBVAR( BREAKPADLIB )
	 OD_MERGE_LIBVAR( BREAKPADCOMMONLIB )
	 OD_MERGE_LIBVAR( BREAKPADCLIENTLIB )
    elseif( APPLE )
	#TODO
    else() # Linux
	find_library( BREAKPADLIB NAMES breakpad
		      PATHS ${BREAKPAD_DIR}/lib
		      REQUIRED )
	find_library( BREAKPADCLIENTLIB NAMES breakpad_client
		      PATHS ${BREAKPAD_DIR}/lib
		      REQUIRED )
    endif()

    set(OD_BREAKPADLIBS ${BREAKPADLIB} ${BREAKPADCOMMONLIB} ${BREAKPADCLIENTLIB} )

    find_program( BREAKPAD_DUMPSYMS_EXECUTABLE NAMES dump_syms 
		  PATHS ${BREAKPAD_DIR}/bin )

    find_program( BREAKPAD_STACKWALK_EXECUTABLE NAMES minidump_stackwalk 
		  PATHS ${BREAKPAD_DIR}/bin )

    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_RELEASE}/symbols
	     DESTINATION ${CMAKE_INSTALL_PREFIX}/${OD_LIB_INSTALL_PATH_RELEASE}
	     CONFIGURATIONS Release )

endif(OD_ENABLE_BREAKPAD)
