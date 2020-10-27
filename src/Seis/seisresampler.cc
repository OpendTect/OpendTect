/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jul 2004
 * FUNCTION : Seismic data keys
-*/


#include "seisresampler.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "cubesubsel.h"
#include "linesubsel.h"
#include "math2.h"


#define mDefConstr(typ) \
 \
SeisResampler::SeisResampler( const typ& inp, \
			      const value_rg_type* vrg ) \
    : worktrc_(*new SeisTrc) \
    , rsd_(*new RangeSelData(inp)) \
{ \
    init( vrg ); \
}

mDefConstr(CubeSubSel)
mDefConstr(LineSubSelSet)
mDefConstr(TrcKeyZSampling)


void SeisResampler::init( const value_rg_type* vrg )
{
    valrg_ = vrg ? new Interval<float>(*vrg) : 0;
    if ( valrg_ )
    {
	valrg_->sort();
	replval_ = (valrg_->start + valrg_->stop) * .5f;
    }
}


SeisResampler::SeisResampler( const RangeSelData& rsd,
			      const value_rg_type* vrg )
    : worktrc_(*new SeisTrc)
    , rsd_(*new RangeSelData(rsd))
{
    init( vrg );
}


SeisResampler::SeisResampler( const SeisResampler& oth )
    : worktrc_(*new SeisTrc)
    , rsd_(*new RangeSelData)
{
    *this = oth;
}


SeisResampler& SeisResampler::operator =( const SeisResampler& oth )
{
    if ( this != &oth )
    {
	worktrc_ = oth.worktrc_;
	rsd_ = oth.rsd_;
	delete valrg_;
	nrtrcs_ = oth.nrtrcs_;
	valrg_ = oth.valrg_ ? new value_rg_type(*oth.valrg_) : 0;
	replval_ = oth.replval_;
	dozsubsel_ = oth.dozsubsel_;
    }

    return *this;
}


SeisResampler::~SeisResampler()
{
    delete valrg_;
    delete &worktrc_;
    delete &rsd_;
}


bool SeisResampler::is2D() const
{
    return rsd_.is2D();
}


SeisTrc* SeisResampler::doWork( const SeisTrc& intrc )
{
    if ( !rsd_.isOK(intrc.info().trcKey()) )
	return 0;

    if ( nrtrcs_ == 0 )
	worktrc_ = intrc; // set data type and components

    nrtrcs_++;
    if ( rsd_.fullSubSel().hasFullZRange() && !valrg_ )
	return const_cast<SeisTrc*>( &intrc );

    worktrc_.info() = intrc.info();
    const auto zrg = rsd_.zRangeFor( intrc.info().geomID() ) ;
    const auto nrsamps = zrg.nrSteps() + 1;
    worktrc_.reSize( nrsamps, false );
    worktrc_.info().sampling_.start = zrg.start;
    worktrc_.info().sampling_.step = zrg.step;

    for ( int icomp=0; icomp<worktrc_.data().nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    const float t = zrg.atIndex( isamp );
	    float val = intrc.getValue( t, icomp );
	    if ( valrg_ )
	    {
		if ( mIsUdf(val) || !Math::IsNormalNumber(val) )
		    val = replval_;
		else if ( val < valrg_->start )
		    val = valrg_->start;
		else if ( val > valrg_->stop )
		    val = valrg_->stop;
	    }
	    worktrc_.set( isamp, val, icomp );
	}
    }

    return &worktrc_;
}
