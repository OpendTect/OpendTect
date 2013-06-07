#_______________________Pmake___________________________________________________
#
#	Makefile : 	EventFreq plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012/03/29 11:33:24 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Attributes)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	eventfreqattrib.cc
	eventfreqpi.cc)

SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} )
SET( OD_NO_ALO_ENTRY 1 )

OD_INIT_MODULE()
