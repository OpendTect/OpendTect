#_______________________Pmake___________________________________________________
#
#	Makefile : 	Algorithms
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt 32184 2013-10-30 11:27:28Z kristofer.tingdahl@dgbes.com $
#_______________________________________________________________________________

set(OD_MODULE_DEPS Basic )
set( OD_TEST_PROGS velocitycalc.cc simpnumer.cc )
set( OD_NIGHTLY_TEST_PROGS fourier.cc bendpoints2coords.cc )

set(OD_MODULE_SOURCES
	    ailayer.cc
	    array1dinterpol.cc
	    array2dinterpol.cc
            arrayndalgo.cc
	    bendpointfinder.cc
	    bendpoints2coords.cc
	    binidsorting.cc
	    bodyvolumecalc.cc
	    conncomponents.cc
	    convolve2d.cc
	    curvature.cc
	    dataclipper.cc
	    delaunay.cc
	    delaunay3d.cc
	    extremefinder.cc
	    dippca.cc
	    fftfilter.cc
	    fourier.cc
	    fourierinterpol.cc
	    genericnumer.cc
	    gridder2d.cc
	    hilberttransform.cc
	    histequalizer.cc
	    houghtransform.cc
	    initalgo.cc
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
	    resizeimage.cc
	    scaler.cc
	    spectrogram.cc
	    staticsdesc.cc
	    stats.cc
	    reflectivitysampler.cc
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
	    zoeppritzcoeff.cc )

OD_INIT_MODULE()
