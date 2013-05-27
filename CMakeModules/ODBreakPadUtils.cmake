#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Aneesh
#	RCS :		$Id$
#_______________________________________________________________________________


set( BREAKPAD_DIR "" CACHE PATH "BREAKPAD Location" )
option ( OD_ENABLE_BREAKPAD "Use breakpad" )

if(OD_ENABLE_BREAKPAD)
	add_definitions( -DHAS_BREAKPAD )
endif(OD_ENABLE_BREAKPAD)

macro(OD_SETUP_BREAKPAD)

	if(OD_ENABLE_BREAKPAD)
		if( WIN32 )
			find_library( BREAKPADLIB_DEBUG NAMES exception_handler 
				      PATHS ${BREAKPAD_DIR}/client/windows/Debug/lib 
				      REQUIRED )
			find_library( BREAKPADLIB_RELEASE NAMES exception_handler 
				      PATHS ${BREAKPAD_DIR}/client/windows/Release/lib 
				      REQUIRED )
			find_library( BREAKPADCOMMONLIB_DEBUG NAMES common 
				      PATHS ${BREAKPAD_DIR}/client/windows/Debug/lib 
				      REQUIRED )
			find_library( BREAKPADCOMMONLIB_RELEASE NAMES common 
				      PATHS ${BREAKPAD_DIR}/client/windows/Release/lib 
				      REQUIRED )
			find_library( BREAKPADCLIENTLIB_DEBUG NAMES crash_generation_client 
				      PATHS ${BREAKPAD_DIR}/client/windows/Debug/lib 
				      REQUIRED )
			find_library( BREAKPADCLIENTLIB_RELEASE NAMES crash_generation_client 
				      PATHS ${BREAKPAD_DIR}/client/windows/Release/lib 
				      REQUIRED )
			 OD_MERGE_LIBVAR( BREAKPADLIB )
			 OD_MERGE_LIBVAR( BREAKPADCOMMONLIB )
			 OD_MERGE_LIBVAR( BREAKPADCLIENTLIB )
		endif( WIN32 )
		
		set(OD_BREAKPADLIBS ${BREAKPADLIB} ${BREAKPADCOMMONLIB} ${BREAKPADCLIENTLIB} )
		list(APPEND OD_MODULE_INCLUDESYSPATH ${BREAKPAD_DIR})
		list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_BREAKPADLIBS} )
	endif(OD_ENABLE_BREAKPAD)

endmacro(OD_SETUP_BREAKPAD)
