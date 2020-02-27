#_______________________Pmake___________________________________________________
#
#	Makefile :	Plugin 'CEEMD - Complete Ensemble Empirical Mode Decomposition'
#	Apr 2012	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/04/17 11:29:00 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	ceemdattribpi.cc
	ceemdattrib.cc
	ceemdalgo.cc
	ceemdtestprogram.h)

SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
