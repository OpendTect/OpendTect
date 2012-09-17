#_______________________Pmake___________________________________________________
#
#	Makefile : 	VMB::VelocityFunction
# 	2008		Kris
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Velocity)
SET(OD_MODULE_SOURCES
		initvolumeprocessing.cc
		velocitygridder.cc
		volprocchain.cc
		volprocattrib.cc
		volproclateralsmoother.cc
		volprocsmoother.cc
		volproctrans.cc
		volprochorinterfiller.cc
		volprocbodyfiller.cc
		volprocvolreader.cc
)
SET(OD_MODULE_BATCHPROGS od_process_volume )
OD_INIT_MODULE()

