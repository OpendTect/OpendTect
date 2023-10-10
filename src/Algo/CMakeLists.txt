#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Basic )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	ailayer.cc
	array1dinterpol.cc
	array2dconverter.cc
	array2dinterpol.cc
	arrayndalgo.cc
	bendpointfinder.cc
	bendpoints2coords.cc
	binidsorting.cc
	bodyvolumecalc.cc
	checksum.cc
	conncomponents.cc
	contcurvinterpol.cc
	convolve2d.cc
	curvature.cc
	dataclipper.cc
	delaunay.cc
	delaunay3d.cc
	dippca.cc
	dragcontroller.cc
	extremefinder.cc
	fftfilter.cc
	fourier.cc
	fourierinterpol.cc
	gaussianprobdenfunc.cc
	genericnumer.cc
	gridder2d.cc
	hilberttransform.cc
	histequalizer.cc
	houghtransform.cc
	initalgo.cc
	interpollayermodel.cc
	isocontourtracer.cc
	linear.cc
	mathfunc.cc
	muter.cc
	pca.cc
	phase.cc
	posfilter.cc
	positionlist.cc
	probdenfunc.cc
	raytrace1d.cc
	reflcalc1d.cc
	reflectivitymodel.cc
	reflectivitysampler.cc
	resizeimage.cc
	scaler.cc
	sincinterpolator.cc
	spectrogram.cc
	stats.cc
	timedepthmodel.cc
	timeser.cc
	transform.cc
	trigonometry.cc
	undefarray.cc
	undo.cc
	valseriesevent.cc
	valseriestracker.cc
	velocitycalc.cc
	wavelettrans.cc
	windowfunction.cc
	zaxistransform.cc
	zaxistransformer.cc
	zoeppritzcoeff.cc
	zvalseriesimpl.cc
)

set( OD_TEST_PROGS
	array2dmatrix.cc
	arraymath.cc
	checksum.cc
	contcurvinterpol.cc
	gaussianprobdenfunc.cc
	simpnumer.cc
	sorting.cc
	timedepthmodel.cc
	velocitycalc.cc
)

set( OD_NIGHTLY_TEST_PROGS
	fourier.cc
	bendpoints2coords.cc
)

OD_INIT_MODULE()
