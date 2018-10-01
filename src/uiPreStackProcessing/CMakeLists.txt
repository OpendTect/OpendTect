#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - PreStack Processing
# 	Jul 2000	Kris
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
	uipsviewer2dwin.cc 
	uipsviewer2dposdlg.cc 
	uisemblancealgo.cc
)
set( OD_MODULE_GUI_PROGS od_PreStackMMBatch.cc )

OD_INIT_MODULE()
