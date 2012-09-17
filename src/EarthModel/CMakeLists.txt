#_______________________Pmake___________________________________________________
#
#	Makefile : 	Earth Model
# 	MNov 1999	Kris Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Geometry )
SET(OD_MODULE_SOURCES
		embody.cc
		embodyoperator.cc
		embodytr.cc
		emfact.cc
		emfault.cc
		emfault3d.cc
		emfault3dpainter.cc
		emfaultstickset.cc
		emfaultsticksetflatvieweditor.cc
		emfaultstickpainter.cc
		emfsstofault3d.cc
		emundo.cc
		emhorizon2d.cc
		emhorizon3d.cc
		emhorizonpainter.cc
		emhorizonpainter2d.cc
		emhorizonpainter3d.cc
		emhorizonpreload.cc
		emhorizonztransform.cc
		emhorizonztransformer.cc
		emhorizonutils.cc
		emhor2dto3d.cc
		emioobjinfo.cc
		emmanager.cc
		emmarchingcubessurface.cc
		emobject.cc
		emobjectselremoval.cc
		emrandomposbody.cc
		empolygonbody.cc
		empolygonbodytr.cc
		emposid.cc
		emrandlinegen.cc
		emrowcoliterator.cc
		emsurface.cc
		emsurfaceauxdata.cc
		emsurfaceedgeline.cc
		emsurfaceedgelineimpl.cc
		emsurfacegeometry.cc
		emsurfaceio.cc
		emsurfaceposprov.cc
		emsurfacetr.cc
		emsurfauxdataio.cc
		flthortools.cc
		horizonmerger.cc
		horizonmodifier.cc
		horizonrelation.cc
		horizonscanner.cc
		horizonsorter.cc
		lmkemfaulttransl.cc
		poly2horvol.cc
		initearthmodel.cc
)
OD_INIT_MODULE()


