/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/


#include "prestackprop.h"

#include "flatposdata.h"
#include "idxable.h"
#include "linear.h"
#include "math2.h"
#include "prestackgather.h"
#include "statruncalc.h"


namespace PreStack
{
mDefineEnumUtils(PropCalc,CalcType,"Calculation type")
{
    "Statistics",
    "AVO Attributes",
    0
};

mDefineEnumUtils(PropCalc,AxisType,"Axis transformation")
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

mDefineEnumUtils(PropCalc,LSQType,"Axis type")
{
    "Intercept",
    "Gradient",
    "StdDev of Intercept",
    "StdDev of Gradient",
    "Correlation coefficient",
    0
};


PropCalc::PropCalc( const Setup& s )
    : setup_(s)
    , innermutes_( 0 )
    , outermutes_( 0 )
{
}


PropCalc::~PropCalc()
{
    removeGather();
}


void PropCalc::removeGather()
{
    if ( gather_ )
    {
	if ( DPM(DataPackMgr::FlatID()).haveID( gather_->id() ) )
	    DPM(DataPackMgr::FlatID()).release( gather_->id() );
	gather_ = nullptr;
    }

    if ( angledata_ )
    {
	if ( DPM(DataPackMgr::FlatID()).haveID( angledata_->id() ) )
	    DPM(DataPackMgr::FlatID()).release( angledata_->id() );
	angledata_ = nullptr;
    }

    delete [] innermutes_;
    innermutes_ = outermutes_ = 0;
}


void PropCalc::handleNewGather()
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


void PropCalc::setGather( const PreStack::Gather& gather )
{
    removeGather();
    gather_ = const_cast<PreStack::Gather*>( &gather );
    handleNewGather();
}


void PropCalc::setGather( DataPack::ID id )
{
    removeGather();

    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( id );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather )
    {
	gather_ = gather;
	handleNewGather();
    }
    else if ( dp && DPM(DataPackMgr::FlatID()).haveID( id ) )
	DPM(DataPackMgr::FlatID()).release( id );
}


void PropCalc::setAngleData( const PreStack::Gather& gather )
{
    if ( angledata_ && DPM(DataPackMgr::FlatID()).haveID( angledata_->id() ) )
	DPM(DataPackMgr::FlatID()).release( angledata_->id() );

    angledata_ = const_cast<PreStack::Gather*>( &gather );
    init();
}


void PropCalc::setAngleData( DataPack::ID id )
{
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( id );
    mDynamicCastGet( Gather*, angledata, dp );

    if ( angledata )
    {
	if ( angledata_ && DPM(DataPackMgr::FlatID()).haveID( angledata_->id()))
	    DPM(DataPackMgr::FlatID()).release( angledata_->id() );

	angledata_ = angledata;
	init();
    }
    else if ( DPM(DataPackMgr::FlatID()).haveID( id ) )
	DPM(DataPackMgr::FlatID()).release( id );
}


bool PropCalc::getAngleFromMainGather() const
{
    return !angledata_ && gather_ && gather_->isOffsetAngle();
}


void PropCalc::init()
{
    const bool dostack = setup_.calctype_ == Stats;
    const bool dolsq   = setup_.calctype_ == LLSQ;
    const bool useangle = setup_.useangle_;
    if ( dolsq && useangle )
	const_cast<PropCalc&>( *this ).setup_.offsaxis_ = Sinsq;

    if ( useangle )
    {
	axisvalsrg_.start = setup_.anglerg_.start;
	axisvalsrg_.stop  = setup_.anglerg_.stop;
    }
    else
	axisvalsrg_ = setup_.offsrg_;
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

#ifdef __debug__
	if ( !getAngleFromMainGather() && !angledata_ )
	    { pErrMsg("Wrongly set"); DBG::forceCrash(false); }
#endif
	eps = 1e-2f;
	if ( dostack )
	{
	    if ( anglevalinradians_ )
	    {
		axisvalsrg_.scale( mDeg2RadF );
		eps *= mDeg2RadF;
	    }
	}
	else
	{
	    axisvalsrg_.scale( mDeg2RadF );
	    eps *= mDeg2RadF;
	}
    }
    else
	eps = 1e-1f;

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

    const bool dostack = setup_.calctype_ == Stats;
    const bool useangle = setup_.useangle_;
    const int nroffsets = gather_->size( !gather_->offsetDim() );
    const int nrz = gather_->size( !gather_->zDim() );
    TypeSet<float> axisvals, vals;
    vals.setCapacity( nroffsets, false );
    if ( setup_.calctype_ != Stats )
	axisvals.setCapacity( nroffsets, false );

    const bool scalexvals = useangle
			  ? ( dostack ? false : !anglevalinradians_)
			  : false;

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
		if ( !angledata_ )
		    continue;

		const float* angledata =
		    angledata_->data().getData() +
		    angledata_->data().info().getOffset( itrc, 0 );
		axisval = IdxAble::interpolateReg( angledata, nrz, cursamp,
						   false, 1e-2f );
	    }

	    if ( scalexvals && !mIsUdf(axisval) )
		axisval *= mDeg2RadF;

	    if ( !axisvalsrg_.isUdf() && !axisvalsrg_.includes(axisval,true) )
		continue;

	    const float val = IdxAble::interpolateReg( seisdata, nrz,cursamp,
						       false, 1e-2f );
	    vals += val;
	    if ( setup_.calctype_ != Stats )
		axisvals += axisval;
	}
    }

    return getVal( setup_, vals, axisvals );
}


static void transformAxis( TypeSet<float>& vals, PropCalc::AxisType at )
{
    if ( at == PropCalc::Norm )
	return;

    for ( int idx=0; idx<vals.size(); idx++ )
    {
	const float val = vals[idx];
	switch ( at )
	{
	case PropCalc::Sqr:	vals[idx] = val * val;		break;
	case PropCalc::Log:	vals[idx] = Math::Log( val );	break;
	case PropCalc::Exp:	vals[idx] = Math::Exp( val );	break;
	case PropCalc::Sqrt:	vals[idx] = Math::Sqrt( val );	break;
	case PropCalc::Abs:	vals[idx] = fabs( val );	break;
	case PropCalc::Sinsq:	vals[idx] = sin( val );
				vals[idx] *= vals[idx];		break;
	default:						break;
	}
    }
}


float PropCalc::getVal( const PropCalc::Setup& su,
			      TypeSet<float>& yvals, TypeSet<float>& xvals )
{
    const int nrvals = yvals.size();
    if ( nrvals == 0 )
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
	return float( rc.getValue(su.stattype_) );
    }
    else if ( xvals.size() != nrvals )
	    return mUdf(float);

    rc.addValues( xvals.size(), xvals.arr() );
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

    transformAxis( xvals, su.offsaxis_ );
    LinStats2D ls2d;
    ls2d.use( xvals.arr(), yvals.arr(), nrvals );
    switch ( su.lsqtype_ )
    {
    case A0:		return ls2d.lp.a0;
    case Coeff:		return ls2d.lp.ax;
    case StdDevA0:	return ls2d.sd.a0;
    case StdDevCoeff:	return ls2d.sd.ax;
    default:		return ls2d.corrcoeff;
    }
}

}
