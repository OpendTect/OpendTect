/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "convolveattrib.h"
#include "deltaresampleattrib.h"
#include "dipfilterattrib.h"
#include "energyattrib.h"
#include "evaluateattrib.h"
#include "eventattrib.h"
#include "fingerprintattrib.h"
#include "frequencyattrib.h"
#include "freqfilterattrib.h"
#include "gapdeconattrib.h"
#include "hilbertattrib.h"
#include "instantattrib.h"
#include "matchdeltaattrib.h"
#include "mathattrib.h"
#include "prestackattrib.h"
#include "positionattrib.h"
#include "referenceattrib.h"
#include "reliefattrib.h"
#include "samplevalueattrib.h"
#include "scalingattrib.h"
#include "semblanceattrib.h"
#include "shiftattrib.h"
#include "similarityattrib.h"
#include "specdecompattrib.h"
#include "textureattrib.h"
#include "volstatsattrib.h"

mDefModInitFn(Attributes)
{
    mIfNotFirstTime( return );

    Attrib::Convolve::initClass();
    Attrib::DeltaResample::initClass();
    Attrib::DipFilter::initClass();
    Attrib::Energy::initClass();
    Attrib::Evaluate::initClass();
    Attrib::Event::initClass();
    Attrib::FingerPrint::initClass();
    Attrib::Frequency::initClass();
    Attrib::FreqFilter::initClass();
    Attrib::GapDecon::initClass();
    Attrib::Hilbert::initClass();
    Attrib::Instantaneous::initClass();
    Attrib::MatchDelta::initClass();
    Attrib::Mathematics::initClass();
    Attrib::PSAttrib::initClass();
    Attrib::Position::initClass();
    Attrib::Reference::initClass();
    Attrib::Relief::initClass();
    Attrib::SampleValue::initClass();
    Attrib::Scaling::initClass();
    Attrib::Semblance::initClass();
    Attrib::Shift::initClass();
    Attrib::Similarity::initClass();
    Attrib::SpecDecomp::initClass();
    Attrib::Texture::initClass();
    Attrib::VolStats::initClass();
}
