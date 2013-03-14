#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Velocities
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiSeis Velocity )
SET(OD_MODULE_SOURCES
		inituivelocity.cc
		uilinearveltrans.cc
		uiselectvelocityfunction.cc
		uivelocityfunctionvolume.cc
		uivelocityfunctionstored.cc
		uivelocityfunctionimp.cc
)
OD_INIT_MODULE()
