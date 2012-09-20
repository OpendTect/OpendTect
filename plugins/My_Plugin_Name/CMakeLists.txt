#_______________________Pmake___________________________________________________
#
#	Makefile : 	Plugin 'template'
# 	Apr 2012	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	my_plugin_name_pi.cc
	my_first_separate_source.cc)
SET( OD_NO_ALO_ENTRY 1 )
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
