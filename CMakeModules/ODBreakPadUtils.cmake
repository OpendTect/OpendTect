#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Aneesh
#	RCS :		$Id$
#_______________________________________________________________________________


set( BREAKPAD_DIR "" CACHE PATH "BREAKPAD Location" )

macro(OD_SETUP_BREAKPAD)

	if(OD_USEBREAKPAD)
		if( WIN32 )
			find_library( BREAKPADLIB NAMES BreakPad PATHS ${BREAKPAD_DIR}/client/windows/Debug/lib REQUIRED )
			find_library( BREAKPADCOMMONLIB NAMES BreakPadCommon PATHS ${BREAKPAD_DIR}/client/windows/Debug/lib REQUIRED )
			find_library( BREAKPADCLIENTLIB NAMES BreakPadClient PATHS ${BREAKPAD_DIR}/client/windows/Debug/lib REQUIRED )
		endif( WIN32 )
		
		set(OD_BREAKPADLIBS ${BREAKPADLIB} ${BREAKPADCOMMONLIB} ${BREAKPADCLIENTLIB} )
		add_definitions( -DHAS_BREAKPAD )
		list(APPEND OD_MODULE_INCLUDESYSPATH ${BREAKPAD_DIR})
		list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_BREAKPADLIBS} )
	endif(OD_USEBREAKPAD)

endmacro(OD_SETUP_BREAKPAD)
