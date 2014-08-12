#_______________________Pmake___________________________________________________
#
#       Makefile :      uiViewer2D
#       July 2002       Umesh Sinha
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiMPE )
SET(OD_MODULE_SOURCES
	inituiviewer2d.cc
	view2ddata.cc
	view2ddataman.cc
	view2dfault.cc
	view2dfaultss2d.cc
	view2dfaultss3d.cc
	view2dhorizon2d.cc
	view2dhorizon3d.cc
	view2dpickset.cc
	view2dseismic.cc
)

OD_INIT_MODULE()
