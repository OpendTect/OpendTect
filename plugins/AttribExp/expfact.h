#ifndef expfact_h
#define expfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id$
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


#endif
