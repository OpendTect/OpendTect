#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "attribfact.h"

#include "expdeconv.h"
#include "expdiscfilter.h"
#include "expdipview.h"
#include "expnodc.h"
#include "expinverse.h"
#include "expnearsubtract.h"
#include "exppcadip.h"
#include "expspectrum.h"
#include "expvardip.h"
#include "expwavelet1d.h"
#include "expinstdip.h"

mClass(AttribExp) ExperimentalAttribFactory
{
public:

static int addAttribs()
{
    AF().add( new DeConvolveAttrib::Parameters, false );
    AF().add( new DipViewAttrib::Parameters, false );
    AF().add( new DiscFilterAttrib::Parameters, false );
    AF().add( new InverseAttrib::Parameters, false );
    AF().add( new MinVarianceDipAttrib::Parameters, false );
    AF().add( new NearSubtractAttrib::Parameters, false );
    AF().add( new NoDCAttrib::Parameters, false );
    AF().add( new PCADipAttrib::Parameters, false );
    AF().add( new TraceSpectrumAttrib::Parameters, false );
    AF().add( new Wavelet1DAttrib::Parameters, false );
    AF().add( new InstantDipAttrib::Parameters, false );
    return 0;
}

static int id;

};


