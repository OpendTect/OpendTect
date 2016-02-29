#_______________________Pmake___________________________________________________
#
#	Makefile : 	Plugin 'template'
# 	Apr 2012	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	my_plugin_name_pi.cc
	my_first_separate_source.cc)
set( OD_NO_ALO_ENTRY 1 )
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()

