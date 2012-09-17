#_______________________Pmake___________________________________________________
#
#	Makefile : 	Geometry-related classes
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General )
SET(OD_MODULE_SOURCES
		binidsurface.cc
		cubicbeziercurve.cc
		cubicbeziersurface.cc
		explfaultsticksurface.cc
		explicitmarchingcubes.cc
		explplaneintersection.cc
		explpolygonsurface.cc
		faulthorintersect.cc
		faultstickset.cc
		faultsticksurface.cc
		geeditor.cc
		geeditorimpl.cc
		geomelement.cc
		impbodyplaneintersect.cc
		indexedshape.cc
		initgeometry.cc
		marchingcubes.cc
		marchingcubeseditor.cc
		horizon2dline.cc
		horizon3dtesselator.cc
		parametriccurve.cc
		parametricsurface.cc
		picksettr.cc
		polygonsurface.cc
		polygonsurfaceedit.cc
		polyposprovider.cc
		posvecdatasettr.cc
		probdenfunctr.cc
		randomlinegeom.cc
		randomlinetr.cc
		rowcolsurface.cc
		stickseteditor.cc
		survgeometry.cc
		tableposprovider.cc
)
OD_INIT_MODULE()


