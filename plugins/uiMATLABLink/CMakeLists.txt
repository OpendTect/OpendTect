#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiMATLAB plugin
# 	March 2012	Nanne Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________

if ( NOT MATLAB_DIR STREQUAL "" )

    set(OD_MODULE_DEPS uiODMain MATLABLink )
    set(OD_IS_PLUGIN yes)
    set(OD_MODULE_SOURCES
	uimatlablinkpi.cc
	uimatlabstep.cc
    )
    set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
    OD_INIT_MODULE()

endif()
