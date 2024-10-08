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
	, replval(0)
	, dozsubsel(false)
	, worktrc(*new SeisTrc)
	, valrg(v? new Interval<float>(*v) : 0)
	, cs(*new TrcKeyZSampling(c))
	, is3d(!is2d)
{
    cs.normalize();
    if ( valrg )
    {
	valrg->sort();
        replval = (valrg->start_ + valrg->stop_) * .5f;
    }
}


SeisResampler::SeisResampler( const SeisResampler& r )
	: worktrc(*new SeisTrc)
	, valrg(0)
	, cs(*new TrcKeyZSampling(r.cs))
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
        if ( mIsUdf(reqzrg.start_) )
            reqzrg.start_ = trczrg.start_;
        if ( mIsUdf(reqzrg.stop_) )
            reqzrg.stop_ = trczrg.stop_;
        if ( reqzrg.step_ < 1e-8 )
            reqzrg.step_ = trczrg.step_;

        if ( !mIsEqual(reqzrg.start_,trczrg.start_,1e-8)
             || !mIsEqual(reqzrg.stop_,trczrg.stop_,1e-8) )
	    dozsubsel = true;

        if ( reqzrg.step_ > 1.01 * trczrg.step_
             || reqzrg.step_ < 0.99 * trczrg.step_ )
	    dozsubsel = true;
	else
            reqzrg.step_ = trczrg.step_;

	if ( dozsubsel )
	{
	    worktrc = intrc;
	    worktrc.info() = intrc.info();
	    worktrc.info().sampling_.start_ = reqzrg.start_;
	    worktrc.info().sampling_.step_ = reqzrg.step_;
	    const int nrsamps =
		(int)( (reqzrg.stop_-reqzrg.start_)/reqzrg.step_ + 1.5 );
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
    worktrc.info().sampling_.start_ = cs.zsamp_.start_;
    worktrc.info().sampling_.step_ = cs.zsamp_.step_;
    if ( intrc.nrComponents() != worktrc.nrComponents() )
    {
	worktrc.setNrComponents( intrc.nrComponents() );
	worktrc.zero();
    }

    for ( int icomp=0; icomp<worktrc.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
            float t = cs.zsamp_.start_ + isamp * cs.zsamp_.step_;
	    float val = intrc.getValue( t, icomp );
	    if ( valrg )
	    {
		if ( !Math::IsNormalNumber(val) )	val = replval;
                else if ( val < valrg->start_ )	val = valrg->start_;
                else if ( val > valrg->stop_ )	val = valrg->stop_;
	    }
	    worktrc.set( isamp, val, icomp );
	}
    }

    return &worktrc;
}
