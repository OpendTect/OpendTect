#_______________________Pmake___________________________________________________
#
#	Makefile : 	GMT plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 09:33:38 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Batch EarthModel Seis Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	gmtpar.cc
	gmtarray2dinterpol.cc
	gmtbasemap.cc
	gmtclip.cc
	gmtcontour.cc
	gmtfault.cc
	gmtlocations.cc
	gmtcoastline.cc
	gmt2dlines.cc
	initgmt.cc
	gmtprocflow.cc
	gmtpi.cc)
SET ( OD_PLUGIN_EXECS od_main )
SET ( OD_MODULE_BATCHPROGS od_gmtexec )
OD_INIT_MODULE()
