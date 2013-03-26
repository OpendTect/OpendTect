#_______________________Pmake___________________________________________________
#
#	Makefile: 	ui- MPE
# 	May 2002	Nanne Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes MPEEngine )
SET(OD_MODULE_SOURCES
		inituimpe.cc
		mpef3dflatvieweditor.cc
		mpefssflatvieweditor.cc
		uiemhorizoneditor.cc
		uiemobjeditor.cc
		uihorizontracksetup.cc
		uimpe.cc
		uimpepartserv.cc
		horflatvieweditor.cc
		horflatvieweditor2d.cc
		horflatvieweditor3d.cc
)
OD_INIT_MODULE()
