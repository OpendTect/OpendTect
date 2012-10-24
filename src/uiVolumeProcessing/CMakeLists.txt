#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Volume Processing
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-08-20 21:05:52 cvsyuancheng Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiVelocity VolumeProcessing)
SET(OD_MODULE_SOURCES
		inituivolumeprocessing.cc
		uivelocitygridder.cc
		uivolprocchain.cc
                uivolprochorinterfiller.cc 
		uivolprocbatchsetup.cc 
		uivolproclateralsmoother.cc 
		uivolprocbodyfiller.cc 
		uivolprocsmoother.cc 
		uivolprocvolreader.cc
)
OD_INIT_MODULE()
