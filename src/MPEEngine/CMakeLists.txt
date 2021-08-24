#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tracking
# 	April 2004	N. Hemstra
#_______________________________________________________________________________

SET(OD_MODULE_DEPS EarthModel AttributeEngine )
SET(OD_MODULE_SOURCES
	autotracker.cc
	emeditor.cc
	emseedpicker.cc
	emtracker.cc
	faulteditor.cc
	faultstickseteditor.cc
	horizon2dextender.cc
	horizon2dselector.cc
	horizon2dseedpicker.cc
	horizon2dtracker.cc
	horizoneditor.cc
	horizon3dextender.cc
	horizon3dseedpicker.cc
	horizon3dtracker.cc
	horizonadjuster.cc
	initmpeengine.cc
	mpeengine.cc
	mpesetup.cc
	polygonsurfeditor.cc
	sectionadjuster.cc
	sectionextender.cc
	sectionselector.cc
	sectionselectorimpl.cc
	sectiontracker.cc
)

OD_INIT_MODULE()
