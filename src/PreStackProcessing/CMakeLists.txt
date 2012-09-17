#_______________________Pmake___________________________________________________
#
#	Makefile : 	Prestack stuff
# 	2008		K. Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Velocity)
SET(OD_MODULE_SOURCES
		initprestackprocessing.cc
		prestackagc.cc
		prestackevents.cc
		prestackeventio.cc
		prestackeventsapi.cc
		prestackeventascio.cc
		prestackeventtransl.cc
		prestackeventtracker.cc
		prestackgather.cc
		prestackprocessor.cc
		prestackprocessortransl.cc
		prestackanglemute.cc
		prestackanglemutecomputer.cc
		prestackmute.cc
		prestackmuteasciio.cc
		prestackmutedef.cc
		prestackmutedeftransl.cc
		prestackprop.cc
		prestackstacker.cc
		prestacklateralstack.cc
		semblancealgo.cc
	 )
SET(OD_MODULE_BATCHPROGS od_process_prestack)
OD_INIT_MODULE()

