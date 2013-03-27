#_______________________Pmake___________________________________________________
#
#	Makefile : 	Algorithms
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS Basic )
set( OD_TEST_PROGS velocitycalc.cc )

set(OD_MODULE_SOURCES timeser.c
	    array1dinterpol.cc
	    array2dinterpol.cc
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
	    fingervein.cc
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
	    posfilter.cc
	    probdenfunc.cc
	    raytrace1d.cc
	    resizeimage.cc
	    scaler.cc
	    spectrogram.cc
	    stats.cc
	    reflectivitysampler.cc
	    transform.cc
	    trigonometry.cc
	    valseriesevent.cc
	    valseriestracker.cc
	    velocitycalc.cc
	    wavelettrans.cc
	    windowfunction.cc
	    zaxistransform.cc
	    zaxistransformer.cc
	    zoeppritzcoeff.cc )

OD_INIT_MODULE()
