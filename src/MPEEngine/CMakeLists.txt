#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tracking
# 	April 2004	N. Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________


SET(OD_MODULE_DEPS EarthModel AttributeEngine )
SET(OD_MODULE_SOURCES
		autotracker.cc
		emeditor.cc
		emtracker.cc
		faultadjuster.cc
		faulteditor.cc
		faultextender.cc
		faultseedpicker.cc
		faultstickseteditor.cc
		faulttracker.cc
		horizon2dextender.cc
		horizon2dselector.cc
		horizon2dseedpicker.cc
		horizon2dtracker.cc
		horizoneditor.cc
		horizon3dextender.cc
		horizon3dseedpicker.cc
		horizon3dtracker.cc
		initmpeengine.cc
		polygonsurfeditor.cc
		sectionadjuster.cc
		sectionextender.cc
		sectionselector.cc
		sectionselectorimpl.cc
		sectiontracker.cc
		mpeengine.cc
		mpesetup.cc
		trackplane.cc
		horizonadjuster.cc
)

OD_INIT_MODULE()
