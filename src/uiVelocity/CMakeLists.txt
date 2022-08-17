#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

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
