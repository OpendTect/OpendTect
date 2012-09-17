#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Velocities
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiSeis PreStackProcessing )
SET(OD_MODULE_SOURCES
		inituiprestackprocessing.cc
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
		uisemblancealgo.cc
)

OD_INIT_MODULE()
