/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "timedepthmodel.h"

#include "bendpointfinder.h"
#include "idxable.h"
#include "survinfo.h"
#include "valseriesimpl.h"
#include "varlenarray.h"
#include "veldesc.h"
#include "velocitycalc.h"
#include "uistrings.h"

#define mRetNoMem( sz, typ ) \
    return uiStrings::phrCannotAllocateMemory( sz*sizeof(typ) )


TimeDepthModel::TimeDepthModel()
    : times_(0)
    , depths_(0)
    , sz_(0)
{
}


TimeDepthModel::TimeDepthModel( const TimeDepthModel& td )
    : times_(0)
    , depths_(0)
{
    setModel( td.depths_, td.times_, td.sz_ );
}



TimeDepthModel::~TimeDepthModel()
{
    delete [] times_;
    delete [] depths_;
}


TimeDepthModel& TimeDepthModel::operator=( const TimeDepthModel& td )
{
    setModel( td.depths_, td.times_, td.sz_ );
    return *this;
}


bool TimeDepthModel::isOK() const
{
    return times_ && depths_ && sz_ > 0;
}


#ifdef __debug__
# define mChkIdx \
    if ( idx >= sz_ ) \
    { \
	BufferString msg("sz_=",sz_, " asked "); msg.add( idx ); \
	pErrMsg(msg); idx = sz_ - 1; \
    }
#else
# define mChkIdx \
    if ( idx >= sz_ ) \
	idx = sz_ - 1;
#endif


float TimeDepthModel::getDepth( float time ) const
{
    return isOK() ? (float)convertTo( depths_, times_, sz_, time, false )
		  : mUdf(float);
}


double TimeDepthModel::getDepthByIdx( int idx ) const
{
    mChkIdx;
    return isOK() ? depths_[idx] : mUdf(double);
}


float TimeDepthModel::getTime( float dpt ) const
{
    return isOK() ? (float)convertTo( depths_, times_, sz_, dpt, true )
		  : mUdf(float);
}


double TimeDepthModel::getTimeByIdx( int idx ) const
{
    mChkIdx;
    return isOK() ? times_[idx] : mUdf(double);
}


float TimeDepthModel::getFirstTime() const
{
    return isOK() ? (float)times_[0] : mUdf(float);
}


float TimeDepthModel::getLastTime() const
{
    return isOK() ? (float)times_[sz_-1] : mUdf(float);
}


float TimeDepthModel::getVelocity( float dpt ) const
{
    return isOK() ? getVelocity( depths_, times_, sz_, dpt ) : mUdf(float);
}


float TimeDepthModel::getDepth( const double* dpths, const double* times,
				int sz, float time )
{
    return (float)convertTo( dpths, times, sz, time, false );
}


float TimeDepthModel::getTime( const double* dpths, const double* times,
			       int sz, float dpt )
{
    return (float)convertTo( dpths, times, sz, dpt, true );
}


void TimeDepthModel::shiftDepths( double shft )
{
    for ( int idx=0; idx<sz_; idx++ )
	depths_[idx] += shft;
}


void TimeDepthModel::shiftTimes( double shft )
{
    for ( int idx=0; idx<sz_; idx++ )
	times_[idx] += shft;
}


float TimeDepthModel::getVelocity( const double* dpths, const double* times,
				   int sz, float depth )
{
    if ( sz < 2 )
	return mUdf(float);

    int idx1;
    IdxAble::findFPPos( dpths, sz, depth, -1, idx1 );
    if ( idx1 < 1 )
	idx1 = 1;
    else if ( idx1 > sz-1 )
	idx1 = sz - 1;

    int idx0 = idx1 - 1;
    const double ddt = times[idx1] - times[idx0];
    const double ddh = dpths[idx1] - dpths[idx0];
    return ddt ? (float)(ddh / ddt) : mUdf(float);
}


double TimeDepthModel::convertTo( const double* dpths, const double* times,
				  int sz, double z, bool targetistime )
{
    const double* zinvals = targetistime ? dpths : times;
    const double* zoutvals = targetistime ? times : dpths;

    int idx1;
    if ( IdxAble::findFPPos( zinvals, sz, z, -1, idx1 ) )
	return zoutvals[idx1];
    else if ( sz < 2 )
	return mUdf(double);
    else if ( idx1 < 0 || idx1 == sz-1 )
    {
	int idx0 = idx1 < 0 ? 1 : idx1;
	const double ddh = dpths[idx0] - dpths[idx0-1];
	const double ddt = times[idx0] - times[idx0-1];
	const double v = ddt ? ddh / ddt : mUdf(double);
	idx0 = idx1 < 0 ? 0 : idx1;
	const double zshift = z - zinvals[idx0];
	const double zout = zoutvals[idx0];
	return targetistime ? ( v ? zout + zshift / v : mUdf(double) )
			    : zout + zshift * v;
    }

    const int idx2 = idx1 + 1;
    const double z1 = z - zinvals[idx1];
    const double z2 = zinvals[idx2] - z;
    return z1+z2 ? (z1 * zoutvals[idx2] + z2 * zoutvals[idx1]) / (z1 + z2)
		 : mUdf(double);
}


uiRetVal TimeDepthModel::setModel( const double* dpths, const double* times,
				   int sz)
{
    deleteAndZeroArrPtr( depths_ );
    deleteAndZeroArrPtr( times_ );

    BendPointBasedMathFunction<double,double> func;
    for ( int idx=0; idx<sz; idx++ )
	func.add( times[idx], dpths[idx] );

    const auto modsz = func.size();
    mTryAlloc( depths_, double[modsz] );
    if ( !depths_ )
	mRetNoMem( modsz, double );
    mTryAlloc( times_, double[modsz] );
    if ( !times_ )
    {
	deleteAndZeroArrPtr( depths_ );
	mRetNoMem( modsz, double );
    }

    sz_ = modsz;
    OD::sysMemCopy( times_, func.xVals().arr(), sz_*sizeof(double) );
    OD::sysMemCopy( depths_, func.yVals().arr(), sz_*sizeof(double) );

    return uiRetVal::OK();
}



void TimeDepthModelSet::getTimes( const float* depths, float* times,
				  int inpsz ) const
{
    const auto sz = size();
    if ( inpsz < 0 )
	inpsz = sz;

    for ( int idx=0; idx<inpsz; idx++ )
    {
	const float dpth = depths[idx];
	times[idx] = mIsUdf(dpth) || inpsz>sz ? mUdf(float)
					      : get(idx)->getTime( dpth );
    }
}


void TimeDepthModelSet::getTimes( const ZValueSet& depths,
				  ZValueSet& times ) const
{
    const auto inpsz = depths.size();
    if ( &depths != &times )
	times.setSize( inpsz, 0.f );

    const auto sz = size();
    for ( int idx=0; idx<inpsz; idx++ )
    {
	const float dpth = depths[idx];
	times[idx] = mIsUdf(dpth) || inpsz>sz ? mUdf(float)
					      : get(idx)->getTime( dpth );
    }
}



TimeDepthConverter::TimeDepthConverter()
    : TimeDepthModel()
    , regularinput_(true)
{
}


bool TimeDepthConverter::isOK() const
{
    return times_ || depths_;
}


bool TimeDepthConverter::isVelocityDescUseable( const VelocityDesc& vd,
					    bool velintime, uiString* emsg )
{
    if ( vd.type_ == VelocityDesc::Avg || vd.type_ == VelocityDesc::Interval )
	return true;

    if ( velintime )
    {
	if ( vd.type_ == VelocityDesc::RMS )
	    return true;

	if ( emsg )
	    *emsg = tr("Sorry, only only RMS, Avg and Interval supported");
	return false;
    }

    if ( emsg )
	*emsg = tr("Sorry, only only Avg and Interval supported");
    return false;
}


#define mIsValidVel( v ) (!mIsUdf(v) && v>1e-3)


uiRetVal TimeDepthConverter::setVelocityModel( const ValueSeries<float>& vel,
				  int sz, const SamplingData<double>& sd,
				  const VelocityDesc& vd, bool istime )
{
    deleteAndZeroArrPtr( times_ );
    deleteAndZeroArrPtr( depths_ );

    PtrMan<ValueSeries<float> > ownvint = 0;
    const ValueSeries<float>* vint = &vel;

    switch ( vd.type_ )
    {
	case VelocityDesc::RMS:
	{
	    if ( !istime )
		return tr("Cannot use RMS velocities in depth");

	    mDeclareAndTryAlloc( float*, ptr, float[sz] );
	    if ( !ptr )
		mRetNoMem( sz, float );

	    ownvint = new ArrayValueSeries<float,float>( ptr, true, sz );
	    if ( !ownvint || !ownvint->isOK() )
		mRetNoMem( sz, float );

	    const float* vrms = vel.arr();
	    ArrPtrMan<float> ownvrms = 0;
	    if ( !vrms )
	    {
		mTryAlloc( ownvrms, float[sz] );
		vrms = ownvrms.ptr();

		if ( vrms )
		{
		    for ( int idx=0; idx<sz; idx++ )
			ownvrms[idx] = vel.value( idx );
		}
	    }

	    if ( !vrms )
		mRetNoMem( sz, float );

	    if ( !computeDix(vrms,sd,sz,ownvint->arr()) )
		break;

	    vint = ownvint.ptr();

	    //Don't break, go into Vint
	}
	case VelocityDesc::Interval:
	{
	    if ( istime )
	    {
		mTryAlloc( depths_, double[sz] );
		if ( !depths_ || !calcDepths(*vint,sz,sd,depths_) )
		    { deleteAndZeroArrPtr( depths_ ); break; }
	    }
	    else
	    {
		mTryAlloc( times_, double[sz] );
		if ( !times_ || !calcTimes(*vint,sz,sd,times_) )
		    { deleteAndZeroArrPtr( times_ ); break; }
	    }

	    for ( int idx=0; idx<sz; idx++ )
	    {
		if ( mIsValidVel(vint->value(idx) ) )
		{
		    firstvel_ = vint->value(idx);
		    break;
		}
	    }

	    lastvel_ = vint->value(sz-1);

	    sz_ = sz;
	    sd_ = sd;
	    break;
	}
	case VelocityDesc::Avg :
	{
	    mAllocVarLenArr( float, vavg, sz );
	    if ( !mIsVarLenArrOK(vavg) )
		mRetNoMem( sz, float );
	    int previdx = -1;
	    float prevvel = 0;
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float curvel = vel.value(idx);
		if ( !mIsValidVel(curvel) )
		    continue;

		vavg[idx] = curvel;
		if ( previdx!=-1 )
		{
		    const float gradient = (curvel-prevvel)/(idx-previdx);
		    for ( int idy=previdx+1; idy<idx; idy++ )
			vavg[idy] = prevvel + (idx-idy)*gradient;
		}
		else
		{
		    for ( int idy=0; idy<idx; idy++ )
		        vavg[idy] = curvel;
		}

		previdx = idx;
		prevvel = curvel;
	    }

	    if ( previdx==-1 )
		break;

	    for ( int idx=previdx+1; idx<sz; idx++ )
		vavg[idx] = prevvel;

	    if ( istime )
	    {
		mTryAlloc( depths_, double[sz] );
		if ( !depths_ )
		    break;

		for ( int idx=0; idx<sz; idx++ )
		    depths_[idx] = sd.atIndex(idx) * vavg[idx]/2.;
	    }
	    else
	    {
		mTryAlloc( times_, double[sz] );
		if ( !times_ )
		    break;

		for ( int idx=0; idx<sz; idx++ )
		    times_[idx] = sd.atIndex(idx) * 2. / vavg[idx];
	    }

	    firstvel_ = vavg[0];
	    lastvel_ = vavg[sz-1];
	    sz_ = sz;
	    sd_ = sd;
	    break;
	}
        default:
            break;
    }

    return uiRetVal::OK();
}


bool TimeDepthConverter::calcDepths( ValueSeries<float>& res, int outputsz,
				    const SamplingData<double>& timesamp ) const
{
    if ( !isOK() )
	return false;

    calcZ( times_, sz_, res, outputsz, timesamp, false );
    return true;
}


bool TimeDepthConverter::calcTimes(ValueSeries<float>& res, int outputsz,
				const SamplingData<double>& depthsamp ) const
{
    if ( !isOK() )
	return false;

    calcZ( depths_, sz_, res, outputsz, depthsamp, true );
    return true;
}


void TimeDepthConverter::calcZ( const double* zvals, int inpsz,
			      ValueSeries<float>& res, int outputsz,
			      const SamplingData<double>& zsamp, bool time)const
{
    double seisrefdatum = SI().seismicReferenceDatum();
    if ( SI().zIsTime() && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorD;

    double* zrevvals = time ? times_ : depths_;
    if ( zrevvals )
    {
	StepInterval<double> zrg;
	if ( !regularinput_ )
	{
	    zrg.start = zvals[0];
	    zrg.stop = zvals[inpsz-1];
	    zrg.step = mUdf(double);
	}
	else
	    zrg = sd_.interval( inpsz );

	for ( int idx=0; idx<outputsz; idx++ )
	{
	    double z = zsamp.atIndex( idx );
	    if ( time ) z += seisrefdatum;

	    double zrev;
	    if ( z <= zrg.start )
	    {
		const double dz = z-zrg.start;
		zrev = time ? firstvel_ > 0 ? (zrevvals[0]+dz*2./firstvel_)
					    : zrevvals[0]
			    : (zrevvals[0] + dz*firstvel_/2.);
	    }
	    else if ( z >= zrg.stop )
	    {
		const double dz = z-zrg.stop;
		zrev = time ?  (zrevvals[inpsz-1] + dz*2./lastvel_)
			    :  (zrevvals[inpsz-1] + dz*lastvel_/2.);
	    }
	    else
	    {
		double zsample;
		if ( regularinput_ )
		{
		    zsample = zrg.getfIndex( z );
		}
		else
		{
		    int sampidx;
		    IdxAble::findPos( zvals, inpsz, z, -1, sampidx );
		    zsample = zvals[sampidx];
		}

		zrev = mCast( double, IdxAble::interpolateReg(zrevvals, inpsz,
				    mCast(float,zsample)) );
	    }

	    if ( !time ) zrev -= seisrefdatum;
	    res.setValue( idx, mCast(float,zrev) );
	}
    }
    else
    {
	int zidx = 0;
	for ( int idx=0; idx<outputsz; idx++ )
	{
	    double z = zsamp.atIndex( idx );
	    if ( time ) z += seisrefdatum;

	    double zrev;
	    if ( z<=zvals[0] )
	    {
		const double dz = z-zvals[0];
		zrev = time ? firstvel_>0 ? (sd_.start+dz*2./firstvel_)
					  : (sd_.start)
			    : (sd_.start+dz*firstvel_/2.);
	    }
	    else if ( z > zvals[inpsz-1] )
	    {
		const double dz = z-zvals[inpsz-1];
		zrev = time ? (sd_.atIndex(inpsz-1)+dz*2./lastvel_)
			    : (sd_.atIndex(inpsz-1)+dz*lastvel_/2.);
	    }
	    else
	    {
		while ( z>zvals[zidx+1] )
		    zidx++;

		const double relidx = zidx +
				    (z-zvals[zidx])/(zvals[zidx+1]-zvals[zidx]);

		zrev = sd_.atIndex( relidx );
	    }

	    if ( !time ) zrev -= seisrefdatum;
	    res.setValue( idx, mCast(float,zrev) );
	}
    }
}



/*!For every time in the velocity model, calculate the depth. The velocity is
   is assumed to be constant at vel[0] above the first time. The velocity is
   specified as Interval velocity, and the sample span is above. That means
   the velocity at a certain sample is the velocity of the layer above the
   corresponding time:
\code
Time(ms) Vel(m/s) Samplespan (ms)	Depth (m)
20	 1000	  16-20			0.020*1000 = 20
24	 1500	  20-24			20+0.004*1500 = 26
28	 2000	  24-28			26+0.004*2000 = 34
\endcode
*/

bool TimeDepthConverter::calcDepths( const ValueSeries<float>& vels, int velsz,
				     const SamplingData<double>& sd,
				     double* depths )
{
    ArrayValueSeries<double,double> times( velsz );
    double* timesptr = times.arr();
    for ( int idx=0; idx<velsz; idx++, timesptr++ )
	*timesptr = sd.atIndex( idx );

    return calcDepths( vels, velsz, times, depths );
}


bool TimeDepthConverter::calcDepths(const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<double>& times,
				    double* depths )
{
    if ( !depths )
	{ pFreeFnErrMsg("No output pointer given" ); return false; }
    if ( velsz < 1 )
	return true;

    double prevvel = mUdf(double);
    int startidx = -1;
    for ( int idx=0; idx<velsz; idx++ )
    {
	if ( !mIsValidVel(vels.value(idx)) )
	    continue;

	startidx = idx;
	prevvel = vels.value(idx);
	break;
    }

    if ( startidx==-1 )
	return false;

    prevvel /= 2.; //time is TWT
    for ( int idx=0; idx<=startidx; idx++ )
	depths[idx] = times.value(idx) * prevvel;

    double depth = depths[startidx];
    for ( int idx=startidx+1; idx<velsz; idx++ )
    {
	double curvel = vels.value( idx );
	if ( mIsValidVel(curvel) )
	    curvel /= 2.;
	else
	    curvel = prevvel;

	depth += (times.value(idx)-times.value(idx-1)) * curvel;

	depths[idx] = depth;
	prevvel = curvel;
    }

    return true;
}


/*!For every depth in the velocity model, calculate the time. The velocity is
   is assumed to be constant at vel[0] above the depth time. The velocity is
   specified as Interval velocity, and the sample span is above. That means
   the velocity at a certain sample is the velocity of the layer above the
   corresponding depth:

\code
Depth(m) Vel(m/s) Samplespan (m)	Time (s)
200	 1000	  160-200		200/1000 = 0.2
240	 1500	  200-240		0.2+40/1500 = 0.23
280	 2000	  240-280		0.23+40/2000 = 0.25

\endcode
*/

bool TimeDepthConverter::calcTimes( const ValueSeries<float>& vels, int velsz,
				    const SamplingData<double>& sd,
				    double* times )
{
    ArrayValueSeries<double,double> depths( velsz );
    double* depthsptr = depths.arr();
    for ( int idx=0; idx<velsz; idx++, depthsptr++ )
	*depthsptr = sd.atIndex( idx );

    return calcTimes( vels, velsz, depths, times );
}


bool TimeDepthConverter::calcTimes( const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<double>& depths,
				    double* times )
{
    if ( !times )
	{ pFreeFnErrMsg( "No output pointer" ); return false; }
    if ( velsz < 1 )
	return true;

    double prevvel = 0;
    int startidx = -1;
    for ( int idx=0; idx<velsz; idx++ )
    {
	if ( !mIsValidVel(vels.value(idx)) )
	    continue;

	startidx = idx;
	prevvel = vels.value(idx);
	break;
    }

    if ( startidx==-1 )
	return false;

    for ( int idx=0; idx<startidx; idx++ )
    {
	double time = depths.value(idx) / prevvel;
	times[idx] = time;
    }

    double time = times[startidx] = depths.value(startidx) / prevvel;

    for ( int idx=startidx+1; idx<velsz; idx++ )
    {
	double curvel = vels.value( idx );
	const double depth = depths.value(idx) - depths.value(idx-1);
	if ( !mIsValidVel(curvel) )
	{
	    curvel = prevvel;
	    if ( curvel>0 )
		time += depth*2./curvel;
	}
	else
	    time += depth*2./curvel; //time is TWT

	times[idx] = time;
	prevvel = curvel;
    }

    return true;
}
