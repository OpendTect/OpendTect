
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2008
-*/


#include "prestackprop.h"

#include "idxable.h"
#include "flatposdata.h"
#include "linear.h"
#include "prestackgather.h"
#include "statruncalc.h"
#include "uistrings.h"

mDefineEnumUtils(PreStack::PropCalc,CalcType,"Calculation type")
{
	"Statistics",
	"AVO Attributes",
	0
};
template<>
void EnumDefImpl<PreStack::PropCalc::CalcType>::init()
{
    uistrings_ += uiStrings::sStatistics();
    uistrings_ += mEnumTr("AVO Attributes",0);
}

mDefineEnumUtils(PreStack::PropCalc,AxisType,"Axis transformation")
{
	"None",
	"Logarithmic",
	"Exponential",
	"Quadratic",
	"Square root",
	"Absolute value",
	"Sine-square",
	0
};
template<>
void EnumDefImpl<PreStack::PropCalc::AxisType>::init()
{
    uistrings_ += uiStrings::sNone();
    uistrings_ += mEnumTr("Logarithmic",0);
    uistrings_ += mEnumTr("Exponential",0);
    uistrings_ += mEnumTr("Quadratic",0);
    uistrings_ += mEnumTr("Square root",0);
    uistrings_ += mEnumTr("Absolute value",0);
    uistrings_ += mEnumTr("Sine-Square",0);

}

mDefineEnumUtils(PreStack::PropCalc,LSQType,"Axis type")
{
	"Intercept",
	"Gradient",
	"StdDev of Intercept",
	"StdDev of Gradient",
	"Correlation coefficient",
	0
};
template<>
void EnumDefImpl<PreStack::PropCalc::LSQType>::init()
{
    uistrings_ += mEnumTr("Intercept",0);
    uistrings_ += uiStrings::sGradient();
    uistrings_ += mEnumTr("StdDev of Intercept",0);
    uistrings_ += mEnumTr("StdDev of Gradient",0);
    uistrings_ += mEnumTr("Correlation Coefficient",0);
}



namespace PreStack
{
PropCalc::PropCalc( const Setup& s )
    : setup_(s)
    , gather_( 0 )
    , innermutes_( 0 )
    , outermutes_( 0 )
    , angledata_( 0 )
{
}


PropCalc::~PropCalc()
{
    removeGather();
}


void PropCalc::removeGather()
{
    delete [] innermutes_;
    innermutes_ = outermutes_ = 0;
}


void PropCalc::setGather( const Gather& gather )
{
    removeGather();
    gather_ = &gather;
    gatherChanged();
}


void PropCalc::gatherChanged()
{
    const int nroffsets = gather_->size( !gather_->offsetDim() );
    mTryAlloc( innermutes_, int[nroffsets*2] );
    if ( innermutes_ )
    {
	outermutes_ = innermutes_ + nroffsets;

	gather_->detectOuterMutes( outermutes_, 0 );
	gather_->detectInnerMutes( innermutes_, 0 );
    }

    init();
}


void PropCalc::setAngleData( const Gather& gather )
{
    angledata_ = &gather;
    init();
}


bool PropCalc::getAngleFromMainGather() const
{
    return !angledata_ && gather_ && gather_->isOffsetAngle();
}


void PropCalc::init()
{
    const bool dostack = setup_.calctype_ == Stats;
    const bool dolsq   = setup_.calctype_ == LLSQ;
    const bool useangle = !setup_.anglerg_.isUdf();
    if ( dolsq && useangle )
	setup_.offsaxis_ = Sinsq;

    axisvalsrg_ = useangle ? setup_.anglerg_ : setup_.offsrg_;
    axisvalsrg_.sort();
    if ( axisvalsrg_.start < 0.f )
	axisvalsrg_.start = 0.f;
    if ( axisvalsrg_.stop < 0.f )
	axisvalsrg_.start = mUdf(float);

    float eps;
    if ( useangle )
    {
	if ( !angledata_ && gather_ && !gather_->isOffsetAngle() )
	    return;

	const bool anglesourceisgather = getAngleFromMainGather();
#ifdef __debug__
	if ( !anglesourceisgather && !angledata_ )
	    { pErrMsg("Wrongly set"); DBG::forceCrash(false); }
#endif
	const Gather::Unit angleunit = anglesourceisgather
				     ? gather_->getXAxisUnit()
				     : angledata_->ampUnit();
	eps = 1e-2f;
	if ( dostack )
	{
	    scalexvals_ = false;
	    if ( angleunit == Gather::Rad )
	    {
		axisvalsrg_.scale( mDeg2RadF );
		eps *= mDeg2RadF;
	    }
	}
	else
	{
	    axisvalsrg_.scale( mDeg2RadF );
	    eps *= mDeg2RadF;
	    scalexvals_ = angleunit == Gather::Deg;
	}
    }
    else
    {
	scalexvals_ = false;
	eps = 1e-1f;
    }

    if ( axisvalsrg_.start > eps )
	axisvalsrg_.start -= eps;
    if ( !mIsUdf(axisvalsrg_.stop) )
	axisvalsrg_.stop += eps;
}


float PropCalc::getVal( int sampnr ) const
{
    if ( !gather_ )
	return mUdf(float);

    const StepInterval<double> si = gather_->posData().range(!gather_->zDim());
    return getVal( (float) si.atIndex(sampnr) );
}


float PropCalc::getVal( float z ) const
{
    if ( !gather_ )
	return mUdf(float);

    const bool useangle = !setup_.anglerg_.isUdf();
    const int nroffsets = gather_->size( !gather_->offsetDim() );
    const int nrz = gather_->size( !gather_->zDim() );

    TypeSet<float> axisvals, vals;
    vals.setCapacity( nroffsets, false );
    if ( setup_.calctype_ != Stats )
	axisvals.setCapacity( nroffsets, false );

    const StepInterval<double> si = gather_->posData().range(!gather_->zDim());
    for ( int itrc=0; itrc<nroffsets; itrc++ )
    {
	const float* seisdata = gather_->data().getData() +
	    gather_->data().info().getOffset(itrc,0);

	for ( int ishft=-setup_.aperture_; ishft<=setup_.aperture_; ishft++ )
	{
	    const float cursamp = ishft+si.getfIndex( z );
	    if ( cursamp>=innermutes_[itrc] || cursamp<=outermutes_[itrc] )
		continue;

	    if ( cursamp<0 || cursamp>=nrz )
		continue;

	    float axisval = mUdf(float);
	    if ( !useangle || getAngleFromMainGather() )
		axisval = gather_->getOffset( itrc );
	    else
	    {
		const float* angledata =
		    angledata_->data().getData() +
		    angledata_->data().info().getOffset( itrc, 0 );
		axisval = IdxAble::interpolateReg( angledata, nrz, cursamp,
						   false, 1e-2f );
	    }

	    if ( scalexvals_ && !mIsUdf(axisval) )
		axisval *= mDeg2RadF;

	    if ( !axisvalsrg_.isUdf() && !axisvalsrg_.includes(axisval,true) )
		continue;

	    const float val =
		IdxAble::interpolateReg( seisdata, nrz,cursamp, false, 1e-2f );
	    vals += val;
	    if ( setup_.calctype_ != Stats )
		axisvals += axisval;
	}
    }

    return getVal( setup_, vals, &axisvals );
}


static void transformAxis( TypeSet<float>& vals, PropCalc::AxisType at )
{
    if ( at == PropCalc::Norm ) return;

    for ( int idx=0; idx<vals.size(); idx++ )
    {
	const float val = vals[idx];
	switch ( at )
	{
	    case PropCalc::Sqr:		vals[idx] = val * val;		break;
	    case PropCalc::Log:		vals[idx] = Math::Log( val );	break;
	    case PropCalc::Exp:		vals[idx] = Math::Exp( val );	break;
	    case PropCalc::Sqrt:	vals[idx] = Math::Sqrt( val );	break;
	    case PropCalc::Abs:		vals[idx] = fabs( val );	break;
	    case PropCalc::Sinsq:	vals[idx] = sin( val );
					vals[idx] *= vals[idx];		break;
	    default:							break;
	}
    }
}


float PropCalc::getVal( const PropCalc::Setup& su, TypeSet<float>& yvals,
			TypeSet<float>* xvals )
{
    const int nrvals = yvals.size();
    if ( nrvals==0 )
	return 0.f;

    transformAxis( yvals, su.valaxis_ );
    Stats::CalcSetup rcs;
    if ( su.calctype_ == Stats )
	rcs.require( su.stattype_ );
    else
	rcs.require( Stats::StdDev );

    Stats::RunCalc<float> rc( rcs );
    if ( su.calctype_ == Stats )
    {
	rc.addValues( nrvals, yvals.arr() );
	return (float) rc.getValue( su.stattype_ );
    }
    else if ( !xvals || xvals->size() != nrvals )
	return mUdf(float);

    rc.addValues( xvals->size(), xvals->arr() );
    if ( mIsZero(rc.getValue(Stats::StdDev),1e-3) )
    {
	Stats::CalcSetup rcsvals;
	rcsvals.require( Stats::StdDev );
	Stats::RunCalc<float> rcvals( rcsvals );
	rcvals.addValues( nrvals, yvals.arr() );
	if ( mIsZero(rcvals.getValue(Stats::StdDev),1e-9) )
	{
	    switch ( su.lsqtype_ )
	    {
		case A0:		return yvals[0];
		case Coeff:		return 0;
		case StdDevA0:		return 0;
		case StdDevCoeff:	return 0;
		default:		return 1;
	    }
	}
    }

    transformAxis( *xvals, su.offsaxis_ );
    LinStats2D ls2d;
    ls2d.use( xvals->arr(), yvals.arr(), nrvals );
    switch ( su.lsqtype_ )
    {
	case A0:		return ls2d.lp.a0_;
	case Coeff:		return ls2d.lp.ax_;
	case StdDevA0:		return ls2d.sd.a0_;
	case StdDevCoeff:	return ls2d.sd.ax_;
	default:		return ls2d.corrcoeff;
    }
}

}
