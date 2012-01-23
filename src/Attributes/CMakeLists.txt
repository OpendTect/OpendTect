#_______________________Pmake___________________________________________________
#
#	Makefile : 	Attribute classes
# 	May 2005	Helene Payraudeau
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine PreStackProcessing)
SET(OD_MODULE_SOURCES
		initattributes.cc
		convolveattrib.cc
		deltaresampleattrib.cc
		dipfilterattrib.cc
		energyattrib.cc
		evaluateattrib.cc
		eventattrib.cc
		fingerprintattrib.cc
		freqfilterattrib.cc
		frequencyattrib.cc
		hilbertattrib.cc
		instantattrib.cc
		matchdeltaattrib.cc
		mathattrib.cc
		prestackattrib.cc
		positionattrib.cc
		referenceattrib.cc
		scalingattrib.cc
		shiftattrib.cc
		similarityattrib.cc
		specdecompattrib.cc
		volstatsattrib.cc
)

OD_INIT_MODULE()


