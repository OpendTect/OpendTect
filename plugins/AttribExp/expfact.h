#ifndef expfact_h
#define expfact_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: expfact.h,v 1.1 2003-12-16 15:44:03 nanne Exp $
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

class ExperimentalAttribFactory
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
    return 0;
}

static int id;

};


#endif
