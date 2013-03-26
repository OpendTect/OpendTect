#_______________________Pmake___________________________________________________
#
#       Makefile :      uiViewer2D
#       July 2002       Umesh Sinha
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiMPE )
SET(OD_MODULE_SOURCES
		inituiviewer2d.cc
		visvw2ddata.cc
		visvw2ddataman.cc
		visvw2dfault.cc
		visvw2dfaultss2d.cc
		visvw2dfaultss3d.cc
		visvw2dhorizon2d.cc
		visvw2dhorizon3d.cc
		visvw2dpickset.cc
		visvw2dseismic.cc
)

OD_INIT_MODULE()
