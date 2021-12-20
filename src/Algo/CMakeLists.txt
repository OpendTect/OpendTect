#_______________________Pmake___________________________________________________
#
#	Makefile : 	Algorithms
# 	July 1995	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS Basic )

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

set( OD_MODULE_SOURCES
	ailayer.cc
	array1dinterpol.cc
	array2dinterpol.cc
	array2dconverter.cc
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
	extremefinder.cc
	dippca.cc
	dragcontroller.cc
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
	reflectivitymodel.cc
	reflectivitysampler.cc
	resizeimage.cc
	scaler.cc
	sincinterpolator.cc
	spectrogram.cc
	staticsdesc.cc
	stats.cc
	timedepthmodel.cc
	timeser.cc
	transform.cc
	trigonometry.cc
	undefarray.cc
	undo.cc
	valseriesevent.cc
	valseriestracker.cc
	veldesc.cc
	velocitycalc.cc
	wavelettrans.cc
	windowfunction.cc
	zaxistransform.cc
	zaxistransformer.cc
	zoeppritzcoeff.cc
)

OD_INIT_MODULE()
