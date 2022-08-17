#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set(OD_MODULE_DEPS AttributeEngine PreStackProcessing)
set(OD_MODULE_SOURCES
	convolveattrib.cc
	deltaresampleattrib.cc
	dipfilterattrib.cc
	energyattrib.cc
	evaluateattrib.cc
	eventattrib.cc
	fingerprintattrib.cc
	freqfilterattrib.cc
	frequencyattrib.cc
	gapdeconattrib.cc
	hilbertattrib.cc
	instantattrib.cc
	matchdeltaattrib.cc
	mathattrib.cc
	prestackattrib.cc
	positionattrib.cc
	referenceattrib.cc
	reliefattrib.cc
	samplevalueattrib.cc
	scalingattrib.cc
	semblanceattrib.cc
	shiftattrib.cc
	similarityattrib.cc
	specdecompattrib.cc
	textureattrib.cc
	volstatsattrib.cc
	initattributes.cc
)

set(OD_MODULE_BATCHPROGS od_process_attrib.cc  )

OD_INIT_MODULE()
