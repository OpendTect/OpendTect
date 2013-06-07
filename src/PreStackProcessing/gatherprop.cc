/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id$";

#include "prestackgatherpropcalc.h"

#include "seispsread.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "statruncalc.h"
#include "linear.h"
#include "math2.h"


namespace PreStack
{

DefineEnumNames(GatherPropCalc,CalcType,1,"Calculation type")
{
	"Statistics",
	"Least-Square",
	0
};

DefineEnumNames(GatherPropCalc,AxisType,2,"Axis transformation")
{
	"None",
	"Logarithmic",
	"Exponential",
	"Quadratic",
	"Square root",
	"Absolute value",
	0
};

DefineEnumNames(GatherPropCalc,LSQType,0,"Axis type")
{
	"Intercept",
	"Gradient",
	"StdDev of Intercept",
	"StdDev of Gradient",
	"Correlation coefficient",
	0
};


GatherPropCalc::GatherPropCalc( const SeisPSReader& r, const Setup& s )
    : rdr_(r)
    , setup_(s)
    , tbuf_(*new SeisTrcBuf(true))
{
}


GatherPropCalc::~GatherPropCalc()
{
    delete &tbuf_;
}


bool GatherPropCalc::goTo( const BinID& bid )
{
    tbuf_.deepErase();
    if ( !rdr_.getGather(bid,tbuf_) )
	return false;

    static const float eps = 1e-3;
    Interval<float> offsrg( setup_.offsrg_ );
    offsrg.sort();
    if ( offsrg.start < eps && offsrg.stop > 1e20 )
	return true;

    for ( int idx=0; idx<tbuf_.size(); idx++ )
    {
	const float offs = tbuf_.get(idx)->info().offset;
	if ( offs < offsrg.start - eps || offs > offsrg.stop + eps )
	{
	    delete tbuf_.remove( idx );
	    idx--;
	}
    }
    return true;
}


float GatherPropCalc::getVal( int sampnr ) const
{
    if ( tbuf_.isEmpty() || tbuf_.get(0)->size() <= sampnr+setup_.aperture_ )
	return mUdf(float);

    TypeSet<float> offs, vals;
    for ( int ishft=-setup_.aperture_; ishft<=setup_.aperture_; ishft++ )
    {
	for ( int itrc=0; itrc<tbuf_.size(); itrc++ )
	{
	    const SeisTrc& trc = *tbuf_.get( itrc );
	    vals += trc.get( sampnr + ishft, setup_.component_ );
	    if ( setup_.calctype_ != Stats )
		offs += setup_.useazim_ ? trc.info().azimuth :trc.info().offset;
	}
    }

    return getVal( setup_, vals, offs );
}


float GatherPropCalc::getVal( float z ) const
{
    if ( tbuf_.isEmpty() )
	return mUdf(float);

    TypeSet<float> offs, vals;
    const float sr = tbuf_.get(0)->info().sampling.step;
    for ( int ishft=-setup_.aperture_; ishft<=setup_.aperture_; ishft++ )
    {
	const float zshft = ishft * sr;
	for ( int itrc=0; itrc<tbuf_.size(); itrc++ )
	{
	    const SeisTrc& trc = *tbuf_.get( itrc );
	    vals += trc.getValue( z + zshft, setup_.component_ );
	    if ( setup_.calctype_ != Stats )
		offs += setup_.useazim_ ? trc.info().azimuth :trc.info().offset;
	}
    }

    return getVal( setup_, vals, offs );
}


static void transformAxis( TypeSet<float>& vals, GatherPropCalc::AxisType at )
{
    if ( at == GatherPropCalc::Norm ) return;

    for ( int idx=0; idx<vals.size(); idx++ )
    {
	const float val = vals[idx];
	switch ( at )
	{
	case GatherPropCalc::Sqr:	vals[idx] = val * val;		break;
	case GatherPropCalc::Log:	vals[idx] = Math::Log( val );	break;
	case GatherPropCalc::Exp:	vals[idx] = Math::Exp( val );	break;
	case GatherPropCalc::Sqrt:	vals[idx] = Math::Sqrt( val );	break;
	case GatherPropCalc::Abs:	vals[idx] = fabs( val );	break;
	}
    }
}


float GatherPropCalc::getVal( const GatherPropCalc::Setup& su,
			      TypeSet<float>& vals, TypeSet<float>& offs )
{
    transformAxis( vals, su.valaxis_ );
    if ( su.calctype_ == Stats )
    {
	Stats::CalcSetup rcs; rcs.require( su.stattype_ );
	Stats::RunCalc<float> rc( rcs );
	rc.addValues( vals.size(), vals.arr() );
	return rc.getValue( su.stattype_ );
    }

    transformAxis( offs, su.offsaxis_ );
    LinStats2D ls2d;
    ls2d.use( offs.arr(), vals.arr(), vals.size() );
    switch ( su.lsqtype_ )
    {
    case A0:		return ls2d.lp.a0;
    case Coeff:		return ls2d.lp.ax;
    case StdDevA0:	return ls2d.sd.a0;
    case StdDevCoeff:	return ls2d.sd.ax;
    default:		return ls2d.corrcoeff;
    }
}

} // namespace PreStack
