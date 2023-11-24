#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Geometry )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	em2ddatafmtconverter.cc
	embody.cc
	embodyoperator.cc
	embodytr.cc
	emfact.cc
	emfault.cc
	emfault3d.cc
	emfault3dpainter.cc
	emfaultauxdata.cc
	emfaultset3d.cc
	emfaultstickpainter.cc
	emfaultstickset.cc
	emfaultsticksetflatvieweditor.cc
	emfsstofault3d.cc
	emhor2dto3d.cc
	emhorizon2d.cc
	emhorizon3d.cc
	emhorizonpainter2d.cc
	emhorizonpainter3d.cc
	emhorizonpreload.cc
	emhorizonutils.cc
	emhorizonztransform.cc
	emhorizonztransformer.cc
	emioobjinfo.cc
	emmanager.cc
	emmarchingcubessurface.cc
	emobject.cc
	emobjectposselector.cc
	empolygonbody.cc
	emposid.cc
	emrandlinegen.cc
	emrandomposbody.cc
	emregion.cc
	emrowcoliterator.cc
	emstoredobjaccess.cc
	emsurface.cc
	emsurfaceauxdata.cc
	emsurfacegeometry.cc
	emsurfaceio.cc
	emsurfaceposprov.cc
	emsurfacetr.cc
	emsurfauxdataio.cc
	emsurft2dtransformer.cc
	emundo.cc
	emzmap.cc
	faulttrace.cc
	horizongridder.cc
	horizonmerger.cc
	horizonmodifier.cc
	horizonrelation.cc
	horizonscanner.cc
	horizon2dscanner.cc
	horizonsorter.cc
	isopachmaker.cc
	lmkemfaulttransl.cc
	poly2horvol.cc
	polygonzchanger.cc
	initearthmodel.cc
)

set( OD_BATCH_TEST_PROGS
	emstoredobjaccess.cc
)

set( OD_MODULE_BATCHPROGS
	od_isopach.cc
)

OD_INIT_MODULE()
