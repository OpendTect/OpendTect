/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisresampler.h"
#include "trckeyzsampling.h"
#include "seistrc.h"
#include "math2.h"

SeisResampler::SeisResampler( const TrcKeyZSampling& c, bool is2d,
				const Interval<float>* v )
	: nrtrcs(0)
	, worktrc(*new SeisTrc)
	, valrg(v? new Interval<float>(*v) : 0)
	, cs(*new TrcKeyZSampling(c))
	, is3d(!is2d)
	, dozsubsel(false)
	, replval(0)
{
    cs.normalize();
    if ( valrg )
    {
	valrg->sort();
	replval = (valrg->start + valrg->stop) * .5f;
    }
}


SeisResampler::SeisResampler( const SeisResampler& r )
	: worktrc(*new SeisTrc)
	, cs(*new TrcKeyZSampling(r.cs))
	, valrg(0)
{
    *this = r;
}


SeisResampler& SeisResampler::operator =( const SeisResampler& r )
{
    if ( this == &r ) return *this;

    nrtrcs = r.nrtrcs;
    is3d = r.is3d;
    worktrc = r.worktrc;
    cs = r.cs;
    delete valrg;
    valrg = r.valrg ? new Interval<float>(*r.valrg) : 0;

    replval = r.replval;
    dozsubsel = r.dozsubsel;
    return *this;
}


SeisResampler::~SeisResampler()
{
    delete &worktrc;
    delete valrg;
    delete &cs;
}


SeisTrc* SeisResampler::doWork( const SeisTrc& intrc )
{
    if ( is3d && !cs.hsamp_.includes(intrc.info().trcKey()) )
	return 0;

    if ( nrtrcs == 0 )
    {
	const StepInterval<float> trczrg( intrc.zRange() );
	StepInterval<float> reqzrg( cs.zsamp_ );
	if ( mIsUdf(reqzrg.start) )
	    reqzrg.start = trczrg.start;
	if ( mIsUdf(reqzrg.stop) )
	    reqzrg.stop = trczrg.stop;
	if ( reqzrg.step < 1e-8 )
	    reqzrg.step = trczrg.step;

	if ( !mIsEqual(reqzrg.start,trczrg.start,1e-8)
	  || !mIsEqual(reqzrg.stop,trczrg.stop,1e-8) )
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
	    int nrsamps = (int)( (reqzrg.stop-reqzrg.start)/reqzrg.step + 1.5 );
	    for ( int idx=0; idx<intrc.data().nrComponents(); idx++ )
		worktrc.data().getComponent(idx)->reSize( nrsamps );
	    worktrc.zero();
	    cs.zsamp_ = reqzrg;
	}
    }

    nrtrcs++;
    if ( !dozsubsel && !valrg )
	return const_cast<SeisTrc*>( &intrc );

    const int nrsamps = worktrc.size();
    worktrc.info() = intrc.info();
    worktrc.info().sampling.start = cs.zsamp_.start;
    worktrc.info().sampling.step = cs.zsamp_.step;
    if ( intrc.nrComponents() != worktrc.nrComponents() )
    {
	worktrc.setNrComponents( intrc.nrComponents() );
	worktrc.zero();
    }

    for ( int icomp=0; icomp<worktrc.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    float t = cs.zsamp_.start + isamp * cs.zsamp_.step;
	    float val = intrc.getValue( t, icomp );
	    if ( valrg )
	    {
		if ( !Math::IsNormalNumber(val) )	val = replval;
		else if ( val < valrg->start )	val = valrg->start;
		else if ( val > valrg->stop )	val = valrg->stop;
	    }
	    worktrc.set( isamp, val, icomp );
	}
    }

    return &worktrc;
}
