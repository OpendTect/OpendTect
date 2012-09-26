#_______________________Pmake___________________________________________________
#
#	Makefile : 	Visualisation - OpendTect Specific
# 	Jan 2002	K. Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-24 13:51:14 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS visBase EarthModel MPEEngine Well PreStackProcessing )
SET(OD_MODULE_SOURCES
		initvissurvey.cc
		visemobjdisplay.cc
		visfaultdisplay.cc
		visfaultsticksetdisplay.cc
		vishingeline.cc
		vishorizondisplay.cc
		vishorizon2ddisplay.cc
		vislocationdisplay.cc
		vismarchingcubessurfacedisplay.cc
		vismpe.cc
		vismpeeditor.cc
		vismpeseedcatcher.cc
		vismultiattribsurvobj.cc
		vispicksetdisplay.cc
		visplanedatadisplay.cc
		vispointsetdisplay.cc
		vispolygonbodydisplay.cc
		vispolylinedisplay.cc
		visprestackviewer.cc
		vispseventdisplay.cc
		visrandomposbodydisplay.cc
		visrandomtrackdisplay.cc
		visseis2ddisplay.cc
		vissower.cc
		vissurvobj.cc
		vissurvscene.cc
		vistransmgr.cc
		visvolumedisplay.cc
		viswelldisplay.cc
)
OD_INIT_MODULE()
