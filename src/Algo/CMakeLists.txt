#_______________________Pmake___________________________________________________
#
#	Makefile : 	Algorithms
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.5 2012/08/06 07:06:18 cvshelene Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Basic )

SET(OD_MODULE_SOURCES timeser.c
	    array1dinterpol.cc
	    array2dinterpol.cc
	    bendpointfinder.cc
	    bendpoints2coords.cc
	    binidsorting.cc
	    convolve2d.cc
	    costrans.cc
	    curvature.cc
	    dataclipper.cc
	    delaunay.cc
	    delaunay3d.cc
	    extremefinder.cc
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
