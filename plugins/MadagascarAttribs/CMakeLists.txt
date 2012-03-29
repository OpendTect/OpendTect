#_______________________Pmake___________________________________________________
#
#	Makefile : 	MadagascarAttribs
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-29 11:26:25 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Attributes)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	madagcattrib.cc
	madattribspi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
