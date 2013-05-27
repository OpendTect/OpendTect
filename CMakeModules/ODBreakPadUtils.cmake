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
			foreach( BUILD_TYPE DEBUG RELEASE )
				find_library( BREAKPADLIB_${BUILD_TYPE} NAMES exception_handler 
				      	      PATHS ${BREAKPAD_DIR}/client/windows/${BUILD_TYPE}/lib
				      	      REQUIRED )
				find_library( BREAKPADCOMMONLIB_${BUILD_TYPE} NAMES common 
				      	      PATHS ${BREAKPAD_DIR}/client/windows/${BUILD_TYPE}/lib 
				      	      REQUIRED )
				find_library( BREAKPADCLIENTLIB_${BUILD_TYPE} NAMES crash_generation_client 
				      	      PATHS ${BREAKPAD_DIR}/client/windows/${BUILD_TYPE}/lib 
				      	      REQUIRED )
			 endforeach()

			 OD_MERGE_LIBVAR( BREAKPADLIB )
			 OD_MERGE_LIBVAR( BREAKPADCOMMONLIB )
			 OD_MERGE_LIBVAR( BREAKPADCLIENTLIB )
		endif( WIN32 )
		
		set(OD_BREAKPADLIBS ${BREAKPADLIB} ${BREAKPADCOMMONLIB} ${BREAKPADCLIENTLIB} )
		list(APPEND OD_MODULE_INCLUDESYSPATH ${BREAKPAD_DIR})
		list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_BREAKPADLIBS} )
	endif(OD_ENABLE_BREAKPAD)

endmacro(OD_SETUP_BREAKPAD)
