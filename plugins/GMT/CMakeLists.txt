#_______________________Pmake___________________________________________________
#
#	Makefile : 	GMT plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS EarthModel Seis Well)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
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
set ( OD_MODULE_BATCHPROGS od_gmtexec.cc )
OD_INIT_MODULE()
