#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Aneesh
#	RCS :		$Id$
#_______________________________________________________________________________


set( BREAKPAD_DIR "" cache path "BREAKPAD Location" )

macro(OD_SETUP_BREAKPAD)
	if ( OD_USEBREAKPAD )
		if( WIN32 )
			find_library( BREAKPADLIB names BreakPad paths ${BREAKPAD_DIR}/client/windows/Debug/lib required )
			find_library( BREAKPADCOMMONLIB names BreakPadCommon paths ${BREAKPAD_DIR}/client/windows/Debug/lib required )
			find_library( BREAKPADCLIENTLIB names BreakPadClient paths ${BREAKPAD_DIR}/client/windows/Debug/lib required )
		endif( WIN32 )
		
		set(OD_BREAKPADLIBS ${BREAKPADLIB} ${BREAKPADCOMMONLIB} ${BREAKPADCLIENTLIB} )
		add_definitions( -DHAS_BREAKPAD )
		list(APPEND OD_MODULE_INCLUDESYSPATH ${BREAKPAD_DIR})
		list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_BREAKPADLIBS} )
	endif( OD_USEBREAKPAD )
endmacro(OD_SETUP_BREAKPAD)
