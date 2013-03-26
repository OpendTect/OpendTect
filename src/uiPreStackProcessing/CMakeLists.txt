#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Velocities
# 	Jul 2000	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiSeis PreStackProcessing )
SET(OD_MODULE_SOURCES
		inituiprestackprocessing.cc
		psviewer2dgatherpainter.cc
		uibatchprestackproc.cc 
		uiprestackagc.cc 
		uiprestackmute.cc 
		uiprestackprocessor.cc 
		uiprestackprocessorsel.cc 
		uiprestacklateralstack.cc 
		uiprestackimpmute.cc 
		uiprestackexpmute.cc 
		uiprestackexpevent.cc 
		uiprestackimpevent.cc 
		uiprestackanglemute.cc 
		uiprestackanglemutecomputer.cc 
		uipsviewer2d.cc 
		uipsviewer2dmainwin.cc 
		uipsviewer2dposdlg.cc 
		uipsviewer2dinfo.cc 
		uisemblancealgo.cc
)

OD_INIT_MODULE()
