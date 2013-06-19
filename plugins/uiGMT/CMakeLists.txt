#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGMT plugin
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain GMT )
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
          uigmtadv.cc
	  uigmtarray2dinterpol.cc
	  uigmtbasemap.cc
	  uigmtclip.cc
	  uigmtcoastline.cc
	  uigmtcontour.cc
	  uigmtfaults.cc
	  uigmtlocations.cc
	  uigmtmainwin.cc
	  uigmtoverlay.cc
	  uigmtpolyline.cc
	  uigmtwells.cc
	  uigmt2dlines.cc
	  uigmtrandlines.cc
	  uigmtsymbolpars.cc
	  uigmtpi.cc
	  uigmtinfodlg.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()

#REFERS_QTLIBS := yes
