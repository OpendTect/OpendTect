#_______________________Pmake___________________________________________________
#
#	Makefile : 	Attribute classes
# 	May 2005	Helene Payraudeau
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-08-28 13:24:21 cvsbert Exp $
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
		samplevalueattrib.cc
		scalingattrib.cc
		shiftattrib.cc
		similarityattrib.cc
		specdecompattrib.cc
		volstatsattrib.cc
)

SET(OD_MODULE_BATCHPROGS od_process_attrib  )

OD_INIT_MODULE()


