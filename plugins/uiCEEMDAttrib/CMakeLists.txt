#_______________________Pmake___________________________________________________
#
#	Makefile :	Plugin 'CEEMD -  Complete Ensemble Empirical Mode Decomposition user interface'
#	Nov 2012	Paul de Groot
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/04/17 11:29:00 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes CEEMDAttrib)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uiceemdattrib.cc
	uiceemdattribpi.cc)


SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
