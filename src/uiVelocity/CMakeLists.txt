#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Velocities
# 	Jul 2000	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiSeis Velocity )
SET(OD_MODULE_SOURCES
		inituivelocity.cc
		uiselectvelocityfunction.cc
		uivelocityfunctionvolume.cc
		uivelocityfunctionstored.cc
		uivelocityfunctionimp.cc
)
OD_INIT_MODULE()
