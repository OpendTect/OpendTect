#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Velocities
# 	Jul 2000	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiSeis Velocity )
set(OD_MODULE_SOURCES
		inituivelocity.cc
		uilinearveltrans.cc
		uiselectvelocityfunction.cc
		uivelocityfunctionvolume.cc
		uivelocityfunctionstored.cc
		uivelocityfunctionimp.cc
)
OD_INIT_MODULE()
