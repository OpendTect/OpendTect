/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2004
 * FUNCTION : Seismic data keys
-*/

static const char* rcsID = "$Id: seisresampler.cc,v 1.2 2004-09-20 16:17:37 bert Exp $";

#include "seisresampler.h"
#include "cubesampling.h"
#include "seistrc.h"

SeisResampler::SeisResampler( const CubeSampling& c, bool is2d,
				const Interval<float>* v )
    	: nrtrcs(0)
    	, worktrc(*new SeisTrc)
    	, valrg(v? new Interval<float>(*v) : 0)
    	, cs(*new CubeSampling(c))
	, is3d(!is2d)
{
    cs.normalise();
    if ( valrg )
    {
	valrg->sort();
	replval = (valrg->start + valrg->stop) * .5;
    }
}


SeisResampler::~SeisResampler()
{
    delete &worktrc;
    delete valrg;
    delete &cs;
}


SeisTrc* SeisResampler::doWork( const SeisTrc& intrc )
{
    if ( (is3d && !cs.hrg.includes(intrc.info().binid))
      || (!is3d && !cs.hrg.includes(BinID(cs.hrg.start.inl,intrc.info().nr))) )
	return 0;

    if ( nrtrcs == 0 )
    {
	StepInterval<float> trczrg( intrc.info().sampling.start,
				    intrc.samplePos( intrc.size(0)-1, 0 ),
				    intrc.info().sampling.step );
	StepInterval<float> reqzrg( cs.zrg );
	if ( reqzrg.step < 1e-8 ) reqzrg.step = trczrg.step;
	if ( reqzrg.start < trczrg.start + 1e-8 )
	    reqzrg.start = trczrg.start;
	else
	    dozsubsel = true;
	if ( reqzrg.stop > trczrg.stop - 1e-8 )
	    reqzrg.stop = trczrg.stop;
	else
	    dozsubsel = true;

	if ( reqzrg.step > 1.01 * trczrg.step
	  || reqzrg.step < 0.99 * trczrg.step )
	    dozsubsel = true;
	else
	    reqzrg.step = trczrg.step;

	if ( dozsubsel )
	{
	    worktrc = intrc;
	    worktrc.info() = intrc.info();
	    worktrc.info().sampling.start = reqzrg.start;
	    worktrc.info().sampling.step = reqzrg.step;
	    int nrsamps = (int)( (reqzrg.stop-reqzrg.start)/reqzrg.step + 1e-4 )
			+ 1;
	    for ( int idx=0; idx<intrc.data().nrComponents(); idx++ )
		worktrc.data().getComponent(idx)->reSize( nrsamps );
	    cs.zrg = reqzrg;
	}
    }

    nrtrcs++;
    if ( !dozsubsel && !valrg )
	return const_cast<SeisTrc*>( &intrc );

    const int nrsamps = worktrc.size(0);
    worktrc.info() = intrc.info();
    worktrc.info().sampling.step = cs.zrg.step;
    for ( int icomp=0; icomp<worktrc.data().nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    float t = cs.zrg.start + isamp * cs.zrg.step;
	    float val = intrc.getValue( t, icomp );
	    if ( valrg )
	    {
		if ( !isFinite(val) )		val = replval;
		else if ( val < valrg->start )	val = valrg->start;
		else if ( val > valrg->stop )	val = valrg->stop;
	    }
	    worktrc.set( isamp, val, icomp );
	}
    }

    return &worktrc;
}
