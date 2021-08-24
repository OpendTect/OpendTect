#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Velocities
# 	Jul 2000	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiSeis PreStackProcessing )
set(OD_MODULE_SOURCES
	inituiprestackprocessing.cc
	psviewer2dgatherpainter.cc
	uibatchprestackproc.cc 
	uiprestackagc.cc 
	uiprestackanglemute.cc 
	uiprestackanglemutecomputer.cc 
	uiprestackexpevent.cc 
	uiprestackexpmute.cc 
	uiprestackimpevent.cc 
	uiprestackimpmute.cc 
	uiprestacklateralstack.cc 
	uiprestackmmproc.cc
	uiprestackmute.cc 
	uiprestackprocessor.cc 
	uiprestackprocessorsel.cc 
	uiprestacktrimstatics.cc 
	uipsviewer2d.cc 
	uipsviewer2dinfo.cc 
	uipsviewer2dmainwin.cc 
	uipsviewer2dposdlg.cc 
	uisemblancealgo.cc
)
SET( OD_MODULE_GUI_PROGS od_PreStackMMBatch.cc )

OD_INIT_MODULE()
