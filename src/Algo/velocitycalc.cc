/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "velocitycalc.h"

#include "bendpointfinder.h"
#include "genericnumer.h"
#include "idxable.h"
#include "interpol1d.h"
#include "math2.h"
#include "survinfo.h"
#include "valseriesimpl.h"
#include "varlenarray.h"
#include "veldesc.h"
#include "limits.h"
#include "uistrings.h"

mImplClassFactory( Vrms2Vint, factory );


TimeDepthModel::TimeDepthModel()
    : errmsg_(uiString::empty())
    , times_(0)
    , depths_(0)
    , sz_(0)
{}


TimeDepthModel::TimeDepthModel( const TimeDepthModel& td )
    : errmsg_(uiString::empty())
    , times_(0)
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
{ return times_ && depths_ && sz_ > 0; }


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
{ return isOK() ? mCast(float,convertTo( depths_, times_, sz_, time, false ))
		 : mUdf(float);}


double TimeDepthModel::getDepth( int idx ) const
{ mChkIdx; return isOK() ? depths_[idx] : mUdf(double); }


float TimeDepthModel::getTime( float dpt ) const
{ return isOK() ? mCast(float,convertTo( depths_, times_, sz_, dpt, true ))
		: mUdf(float); }


double TimeDepthModel::getTime( int idx ) const
{ mChkIdx; return isOK() ? times_[idx] : mUdf(double); }


float TimeDepthModel::getFirstTime() const
{ return isOK() ? mCast(float,times_[0]) : mUdf(float); }


float TimeDepthModel::getLastTime() const
{ return isOK() ? mCast(float,times_[sz_-1]) : mUdf(float); }


float TimeDepthModel::getVelocity( float dpt ) const
{ return isOK() ? getVelocity( depths_, times_, sz_, dpt ) : mUdf(float); }


float TimeDepthModel::getDepth( const double* dpths, const double* times,
				int sz, float time )
{ return mCast(float,convertTo( dpths, times, sz, time, false )); }


float TimeDepthModel::getTime( const double* dpths, const double* times,
			       int sz, float dpt )
{ return mCast(float,convertTo( dpths, times, sz, dpt, true )); }


float TimeDepthModel::getVelocity( const double* dpths, const double* times,
				   int sz, float depth )
{
    if ( sz < 2 )
	return mUdf(float);

    int idx1;
    IdxAble::findFPPos( dpths, sz, mCast(double,depth), -1, idx1 );
    if ( idx1 < 1 )
	idx1 = 1;
    else if ( idx1 > sz-1 )
	idx1 = sz - 1;

    int idx0 = idx1 - 1;
    const double ddt = times[idx1] - times[idx0];
    const double ddh = dpths[idx1] - dpths[idx0];
    return ddt ? mCast(float,ddh / ddt) : mUdf(float);
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


bool TimeDepthModel::setModel( const double* dpths, const double* times, int sz)
{
    deleteAndZeroArrPtr( depths_ );
    deleteAndZeroArrPtr( times_ );

    BendPointBasedMathFunction<double,double> func;
    for ( int idx=0; idx<sz; idx++ )
	func.add( times[idx], dpths[idx] );

    mTryAlloc( depths_, double[func.size()] );
    if ( !depths_ )
	{ errmsg_ = tr("Out of memory"); return false; }

    mTryAlloc( times_, double[func.size()] );
    if ( !times_ )
	{ errmsg_ = tr("Out of memory"); return false; }

    sz_ = func.size();
    OD::sysMemCopy( times_, func.xVals().arr(), sz_*sizeof(double) );
    OD::sysMemCopy( depths_, func.yVals().arr(), sz_*sizeof(double) );

    return true;
}




TimeDepthConverter::TimeDepthConverter()
    : TimeDepthModel()
    , regularinput_(true)
{}


bool TimeDepthConverter::isOK() const
{ return times_ || depths_; }


bool TimeDepthConverter::isVelocityDescUseable(const VelocityDesc& vd,
					       bool velintime,
					       uiString* errmsg )
{
    if ( vd.type_==VelocityDesc::Avg || vd.type_==VelocityDesc::Interval )
	return true;

    if ( velintime )
    {
	if ( vd.type_==VelocityDesc::RMS )
	    return true;

	if ( errmsg )
	    *errmsg = tr("Only RMS, Avg and Interval allowed for time based "
		         "models");
	return false;
    }

    if ( errmsg )
	*errmsg = tr("Only Avg and Interval allowed for depth based "
		     "models");

    return false;
}


#define mIsValidVel( v ) (!mIsUdf(v) && v>1e-3)


bool TimeDepthConverter::setVelocityModel( const ValueSeries<float>& vel,
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
	    {
		errmsg_ = tr("Cannot use RMS velocities in depth");
		break;
	    }

	    mDeclareAndTryAlloc( float*, ptr, float[sz] );
	    if ( !ptr )
	    {
		errmsg_ = tr("Out of memory");
		break;
	    }

	    ownvint = new ArrayValueSeries<float,float>( ptr, true, sz );
	    if ( !ownvint || !ownvint->isOK() )
	    {
		errmsg_ = tr("Out of memory");
		break;
	    }

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
	    {
		errmsg_ = tr("Out of memory");
		break;
	    }

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
	    if ( !mIsVarLenArrOK(vavg) ) return false;
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

    return isOK();
}


bool TimeDepthConverter::calcDepths(ValueSeries<float>& res, int outputsz,
				    const SamplingData<double>& timesamp ) const
{
    if ( !isOK() ) return false;
    calcZ( times_, sz_, res, outputsz, timesamp, false );
    return true;
}


bool TimeDepthConverter::calcTimes(ValueSeries<float>& res, int outputsz,
				const SamplingData<double>& depthsamp ) const
{
    if ( !isOK() ) return false;
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
    {
	pFreeFnErrMsg("No output pointer given." );
	return false;
    }

    if ( !velsz ) return true;

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
    {
	pFreeFnErrMsg( "No output pointer" );
	return false;
    }

    if ( !velsz ) return true;

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


class MoveoutComputerError : public FloatMathFunction
{
public:
    MoveoutComputerError( const MoveoutComputer& m,float* variables,
	    int variabletochange, int nroffsets,
	    const float* offsets, const float* moveout )
	: calc_( m )
	, variables_( variables )
	, variable_( variabletochange )
	, nroffsets_( nroffsets )
	, realmoveouts_( moveout )
	, calcmoveouts_( new float[nroffsets] )
	, offsets_( offsets )
    {}

    ~MoveoutComputerError() { delete [] calcmoveouts_; }

    float		getValue( const float* pos ) const
			{ return getValue(*pos); }
    float		getValue(float val) const
    {
	variables_[variable_] = val;
	if ( !calc_.computeMoveout( variables_, nroffsets_, offsets_,
	      calcmoveouts_ ) )
	    return mUdf(float);

	float sqsum = 0;
	for ( int idx=0; idx<nroffsets_; idx++ )
	{
	    const float diff = calcmoveouts_[idx]-realmoveouts_[idx];
	    sqsum += diff*diff;
	}

	return sqsum;
    }


protected:
    const MoveoutComputer&	calc_;
    mutable float*		variables_;
    const int			variable_;
    int				nroffsets_;
    const float*		offsets_;
    const float*		realmoveouts_;
    mutable float*		calcmoveouts_;
};



float MoveoutComputer::findBestVariable( float* variables, int variabletochange,
	const Interval<float>& range, int nroffsets, const float* offsets,
	const float* moveout ) const
{
    MoveoutComputerError errorfunc(*this, variables, variabletochange,
	    nroffsets, offsets, moveout );
    const float res = findExtreme( errorfunc, true, range.start, range.stop );
    if ( mIsUdf(res) )
	return res;

    variables[variabletochange] = res;
    return Math::Sqrt( errorfunc.getValue( res ) );
}


bool RMOComputer::computeMoveout( const float* variables, int nroffsets,
	const float* offsets, float* res ) const
{
    return computeMoveout( variables[0], variables[1], variables[2],
	    nroffsets, offsets, res );
}


bool RMOComputer::computeMoveout( float d0, float rmo, float refoffset,
	int nroffsets, const float* offsets, float* res )
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	float ratio = offsets[idx]/refoffset;
	ratio *= ratio;
	res[idx] = d0 + rmo * ratio;
    }

    return true;
}




bool NormalMoveout::computeMoveout( const float* variables, int nroffsets,
	const float* offsets, float* res ) const
{
    return computeMoveout( variables[0], variables[1], variables[2],
	    nroffsets, offsets, res );
}


bool NormalMoveout::computeMoveout( float t0, float Vrms,
	float effectiveanisotropy, int nroffsets, const float* offsets,
	float* res )
{
    const double t0_2 = t0*t0;
    const double v2 = Vrms*Vrms;

    if ( mIsUdf(effectiveanisotropy) )
	effectiveanisotropy = 0;

    const bool hasanisotropy = !mIsZero(effectiveanisotropy, 1e-3);

    for ( int idx=nroffsets-1; idx>=0; idx-- )
    {
	const double offset = offsets[idx];
	const double offset2 = offset*offset;

	double t2 = t0_2 + offset2/v2;
	if ( hasanisotropy )
	{
	    const double offset4 = offset2*offset2;
	    const double numerator = 2 * effectiveanisotropy * offset4;
	    const double denominator =
		v2*(t0_2*v2+(1+2*effectiveanisotropy)*offset2);

	    double anisotropycontrib = 0;
	    if ( denominator>0 )
		anisotropycontrib = numerator/denominator;

	    if ( anisotropycontrib<t2 )
		t2 -= anisotropycontrib;
	}

	if ( t2<=0 )
	    res[idx] = 0;
	else
	    res[idx] = (float) Math::Sqrt( t2 );
    }

    return true;
}


#define mComputeDixImpl( first_t, first_v, timefetch ) \
    if ( !Vrms || !Vint ) \
	return false; \
\
    if ( !nrvels ) \
	return true; \
 \
    int idx_prev = -1; \
    double t_above = -first_t; \
    double v2t_prev = t_above*first_v*first_v;; \
    double vlayer = mUdf(double);\
 \
    for ( int idx=0; idx<nrvels; idx++ ) \
    { \
	const double v = Vrms[idx]; \
	if ( mIsUdf(v) ) \
	    continue; \
 \
	double t_below = timefetch-first_t; \
 \
	const double v2t = t_below*v*v; \
	const double numerator = v2t-v2t_prev; \
	if ( numerator<0 ) \
	    continue; \
 \
	if ( t_below<t_above || mIsEqual(t_below,t_above,1e-5) ) \
	    continue; \
 \
	vlayer = Math::Sqrt( numerator/(t_below-t_above) ); \
 \
	for ( int idy=idx_prev+1; idy<=idx; idy++ ) \
	    Vint[idy] = mCast(float,vlayer); \
 \
	v2t_prev = v2t; \
	t_above = t_below; \
	idx_prev = idx; \
    } \
 \
    for ( int idx=idx_prev+1; idx<nrvels; idx++ )\
	Vint[idx] = mCast(float,vlayer);\
 \
    return true

bool computeDix( const float* Vrms, const SamplingData<double>& sd, int nrvels,
		 float* Vint )
{
    mComputeDixImpl( 0, 0, sd.atIndex(idx) );
}


bool computeDix( const float* Vrms, double t0, float v0, const double* t,
		 int nrvels, float* Vint )
{
    mComputeDixImpl( t0, v0, t[idx] );
}


bool computeVrms( const float* Vint, const SamplingData<double>& sd, int nrvels,
		  float* Vrms )
{
    double t_above = 0;
    int idx_prev = -1;
    double v2t_prev = 0;

    for ( int idx=0; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double t_below = sd.atIndex(idx);

	double dt = t_below - t_above;
	double numerator = v2t_prev+V_interval*V_interval*dt;
	float res = (float) Math::Sqrt( numerator/t_below );

	if ( !Math::IsNormalNumber(res) ) //looks for division by zero above
	    continue;

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vrms[idy] = res;

	v2t_prev = numerator;
	t_above = t_below;
	idx_prev = idx;
    }

    return true;
}


bool computeVrms( const float* Vint, double t0, const double* t, int nrvels,
		  float* Vrms )
{
    double t_above = t0;
    int idx_prev = -1;
    double v2t_prev = 0;

    for ( int idx=0; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double t_below = t[idx];

	double dt = t_below - t_above;
	double numerator = v2t_prev+V_interval*V_interval*dt;
	float res = (float) Math::Sqrt( numerator/(t_below-t0) );
	//TODO: Check whether t0 should be subtracted above

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vrms[idy] = res;

	v2t_prev = numerator;
	t_above = t_below;
	idx_prev = idx;
    }

    return true;
}


bool sampleVrms( const float* Vin, double t0_in, float v0_in,const double* t_in,
		 int nr_in, const SamplingData<double>& sd_out, float* Vout,
		 int nr_out )
{
    if ( nr_out<=0 )
	return true;

    if ( nr_in<=0 )
	return false;

    if ( nr_in==1 )
    {
	for ( int idx=0; idx<nr_out; idx++ )
	    Vout[idx] = Vin[0];

	return true;
    }

    ArrayValueSeries<float,float> Vint ( nr_in );
    if ( !computeDix( Vin, t0_in, v0_in, t_in, nr_in, Vint.arr() ) )
	return false;

    const ArrayValueSeries<double,double> tinser( const_cast<double*>(t_in),
						  false );
    mAllocVarLenArr( double, deptharr, nr_in );
    mAllocVarLenArr( double, depthsampled, nr_out );
    mAllocVarLenArr( float, Vint_sampled, nr_out );
    if ( !tinser.isOK() || !mIsVarLenArrOK(deptharr) ||
	 !mIsVarLenArrOK(depthsampled) || !mIsVarLenArrOK(Vint_sampled) )
	return false;

    TimeDepthConverter::calcDepths( Vint, nr_in, tinser, deptharr );
    if ( nr_in<2 )
	return false;
    else
	resampleZ( deptharr, t_in, nr_in, sd_out, nr_out, depthsampled );

    //compute Vint_sampled from depthsampled
    Vint_sampled[0] = 0;
    const double halfinvstep = 2. / sd_out.step; //time is TWT
    for ( int idx=1; idx<nr_out; idx++ )
    {
	Vint_sampled[idx] = mCast(float, halfinvstep *
				(depthsampled[idx]-depthsampled[idx-1]) );
    }

    return computeVrms( (const float*)Vint_sampled, sd_out, nr_out, Vout );
}


bool computeVavg( const float* Vint, const double* z, int nrvels,
		  float* Vavg )
{
    const bool zistime = SI().zIsTime();

    int idx_prev;
    for ( idx_prev=0; idx_prev<nrvels; idx_prev++ )
    {
	if ( !mIsUdf(Vint[idx_prev]) )
	    break;
    }
    if ( idx_prev == nrvels )
    {
	OD::sysMemValueSet( Vavg, mUdf(float), nrvels );
	return false;
    }

    Vavg[idx_prev] = Vint[idx_prev];
    double z_above = z[idx_prev];
    double v2z_prev = zistime ? Vavg[idx_prev] * z_above
			      : z_above / Vavg[idx_prev];

    for ( int idx=1; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double z_below = z[idx];
	float res = 0;
	double dz = z_below - z_above;

	if ( zistime )
	{
	    double numerator = v2z_prev+V_interval*dz;
	    res = (float) ( numerator/z_below );

	    if ( !Math::IsNormalNumber(res) ) //looks for division by zero above
		continue;

	    v2z_prev = numerator;
	}
	else
	{
	    double denominator = v2z_prev + dz/V_interval;
	    res = (float) ( z_below / denominator );
	    if ( !Math::IsNormalNumber(res) ) //looks for division by zero above
		continue;

	    v2z_prev = denominator;
	}

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vavg[idy] = res;

	z_above = z_below;
	idx_prev = idx;
    }

    return true;
}


bool computeVint( const float* Vavg, const double* z, int nrvels, float* Vint )
{
    const bool zistime = SI().zIsTime();

    int idx_prev;
    for ( idx_prev=0; idx_prev<nrvels; idx_prev++ )
    {
	if ( !mIsUdf(Vavg[idx_prev]) )
	    break;
    }
    if ( idx_prev == nrvels )
    {
	OD::sysMemValueSet( Vint, mUdf(float), nrvels );
	return false;
    }

    Vint[idx_prev] = Vavg[idx_prev];
    double z_above = z[idx_prev];
    double v2z_prev = zistime ? z_above*Vavg[idx_prev] : z_above/Vavg[idx_prev];
    bool hasvals = false;

    for ( int idx=1; idx<nrvels; idx++ )
    {
	const double v = Vavg[idx];
	if ( mIsUdf(v) )
	    continue;

	hasvals = true;

	double z_below = z[idx];

	const double v2z = zistime ? z_below*v : z_below/v;
	const double numerator = v2z-v2z_prev;
	if ( numerator<0 )
	    continue;

	if ( ( zistime && (z_below<z_above || mIsEqual(z_below,z_above,1e-5)) )
	   || ( !zistime && mIsZero(numerator,1e-3) ) )
	    continue;

	const double vlayer = zistime ? numerator/(z_below-z_above)
				      : (z_below-z_above)/numerator;

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vint[idy] = (float) vlayer;

	v2z_prev = v2z;
	z_above = z_below;
	idx_prev = idx;
    }

    if ( !hasvals )
    {
	idx_prev = 0;
	Vint[0] = mUdf(float);
    }

    for ( int idx=idx_prev+1; idx<nrvels; idx++ )
	Vint[idx] = Vint[idx_prev];

    return true;
}


void resampleZ( const double* zarr, const double* tord_in, int nr_in,
		    const SamplingData<double>& sd_out, int nr_out,
		    double* zsampled )
{
    resampleContinuousData( zarr, tord_in, nr_in, sd_out, nr_out, zsampled );
}


void sampleEffectiveThomsenPars( const float* vinarr, const double* t_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* voutarr )
{
    mAllocVarLenArr( double, dvinarr, nr_in );
    mAllocVarLenArr( double, dvoutarr, nr_out );
    if ( !mIsVarLenArrOK(dvinarr) || !mIsVarLenArrOK(dvoutarr) ) return;
    resampleContinuousData( dvinarr, t_in, nr_in, sd_out, nr_out, dvoutarr );
    for ( int idx=0; idx<nr_out; idx++ )
	voutarr[idx] = mCast(float,dvoutarr[idx]);
}


void resampleContinuousData( const double* inarr, const double* t_in, int nr_in,
			     const SamplingData<double>& sd_out, int nr_out,
			     double* outarr )
{
    int intv = 0;
    const double eps = sd_out.step / 1e3;
    for ( int idx=0; idx<nr_out; idx++ )
    {
	bool match = false;
	const double z = sd_out.atIndex( idx );
	for ( ; intv<nr_in; intv++ )
	{
	    if ( mIsEqual(z,t_in[intv],eps) )
		match = true;
	    else if ( t_in[intv]<=z )
		continue;

	    break;
	}

	//intv is always after pos
	if ( match )
	    outarr[idx] = inarr[intv];
	else
	{
	    if ( intv == nr_in )
		intv--;

	    const double v0 = !intv? 0. : inarr[intv-1];
	    const double v1 = inarr[intv];
	    const float t0 = mCast(float,!intv? 0. : t_in[intv-1]);
	    const float t1 = mCast(float,t_in[intv]);
	    outarr[idx] = mIsEqual( v0, v1, 1e-6 ) ? v0 :
			Interpolate::linear1D( t0, v0, t1, v1, mCast(float,z) );
	}
    }
}


bool sampleVint( const float* Vin, const double* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    if ( !nr_in )
	return false;

    const ArrayValueSeries<double,double> tinser( const_cast<double*>(t_in),
						  false );
    const ArrayValueSeries<float,float> Vinser( const_cast<float*>(Vin),
						false );
    mAllocVarLenArr( double, deptharr, nr_in );
    mAllocVarLenArr( double, depthsampled, nr_out );
    if ( !tinser.isOK() || !Vinser.isOK() ||
	 !mIsVarLenArrOK(deptharr) || !mIsVarLenArrOK(depthsampled) )
	return false;

    TimeDepthConverter::calcDepths( Vinser, nr_in, tinser, deptharr );
    if ( nr_in<2 )
	return false;
    else
	resampleZ( deptharr, t_in, nr_in, sd_out, nr_out, depthsampled );

    //compute Vout from depthsampled
    Vout[0] = Vin[0];
    const double halfinvstep = 2. / sd_out.step; //time is TWT
    for ( int idx=1; idx<nr_out; idx++ )
    {
	Vout[idx] = mCast(float, halfinvstep *
				 (depthsampled[idx] - depthsampled[idx-1]) );
    }

    return true;
}


bool sampleVavg( const float* Vin, const double* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    mAllocVarLenArr( float, vintarr, nr_in );
    mAllocVarLenArr( float, vintsampledarr, nr_out );
    mAllocVarLenArr( double, outtimesampsarr, nr_out );
    if ( !mIsVarLenArrOK(vintarr) || !mIsVarLenArrOK(vintsampledarr) ||
	 !mIsVarLenArrOK(outtimesampsarr) )
	return false;

    for ( int idx=0; idx<nr_out; idx++ )
	outtimesampsarr[idx] = sd_out.atIndex( idx );

    return computeVint( Vin, t_in, nr_in, vintarr ) &&
	sampleVint( vintarr,t_in,nr_in, sd_out, vintsampledarr, nr_out ) &&
	computeVavg( vintsampledarr, outtimesampsarr, nr_out, Vout );
}


void computeResidualMoveouts( float z0, float rmo, float refoffset,
			      int nroffsets, bool outputdepth,
			      const float* offsets, float* output )
{
    const bool alludf =
	    (outputdepth && mIsUdf(z0)) || mIsUdf(rmo) || mIsUdf(refoffset) ||
	    mIsZero(refoffset,1e-5);

    for ( int idx=0; idx<nroffsets; idx++, offsets++, output++ )
    {
	if ( alludf )
	{
	    *output = mUdf(float);
	    continue;
	}

	const float offset = *offsets;
	if ( mIsUdf(offset) )
	{
	    *output = mUdf(float);
	    continue;
	}

	const float ratio = offset/refoffset;
	*output = rmo * ratio * ratio;
	if ( outputdepth )
	    *output += z0;
    }
}


void sampleIntvThomsenPars( const float* inarr, const double* t_in, int nr_in,
		    const SamplingData<double>& sd_out, int nr_out,
		    float* outarr )
{
    int intv = 0;
    for ( int idx=0; idx<nr_out; idx++ )
    {
	const double z = sd_out.atIndex( idx );
	for ( ; intv<nr_in; intv++ )
	{
	    if ( t_in[intv]<z )
		continue;

	    break;
	}

	//intv is always after pos
	if ( intv == nr_in )
	    intv--;

	outarr[idx] = inarr[intv];
    }
}


#define mSmallNumber 1e-7


bool computeLinearT2D( double v0, double dv, double v0depth,
		       const SamplingData<float>& sd,
		       int sz, float* res )
{
    if ( sz<=0 )
	return true;

    if ( v0<=0 || mIsUdf(v0) )
	return false;

    const bool zerogradient = mIsZero( dv, 1e-6 ) || mIsUdf(dv);

    if ( zerogradient )
	dv = mSmallNumber;

    const double fact = dv / v0;
    const double dv2 = 2. / dv;

    const double logval = Math::Log( 1. + v0depth * fact );
    if ( mIsUdf(logval) )
	return false;

    const double sealeveltwt = - dv2 * logval;

    for ( int idx=0; idx<sz; idx++ )
    {
	const double time = (sd.start+idx*sd.step-sealeveltwt) / 2.;
	const double expterm = Math::Exp( dv * time );

	if ( mIsUdf(expterm) )
	{
	    res[idx] = mUdf(float);
	    continue;
	}

	const double depth = ( expterm - 1. ) / fact;

	if ( depth>MAXFLOAT )
	    res[idx] = mUdf(float);
	else
	    res[idx] = mCast(float,depth);
    }

    return true;
}


bool computeLinearD2T( double v0, double dv, double v0depth,
		      const SamplingData<float>& sd,
		      int sz, float* res )
{
    if ( sz < 0 )
	return true;

    if ( v0<=0 || mIsUdf(v0) )
	return false;

    const bool zerogradient = mIsZero( dv, 1e-6 ) || mIsUdf(dv);

    if ( zerogradient )
	dv = mSmallNumber;

    const double fact = dv / v0;
    const double dv2 = 2. / dv;
    double logval = Math::Log( 1. + v0depth * fact );
    if ( mIsUdf(logval) )
	return false;

    const double sealeveltwt = - dv2 * logval;

    for ( int idx=0; idx<sz; idx++ )
    {
	const double depth = sd.start + idx*sd.step;
	logval = Math::Log(1.+depth*fact);
	if ( mIsUdf(logval) )
	    res[idx] = mUdf(float);
	else
	    res[idx] = mCast(float,dv2 * logval+sealeveltwt);
    }

    return true;
}


bool convertToVintIfNeeded( const float* inpvel, const VelocityDesc& veldesc,
			    const StepInterval<float>& zrange, float* outvel )
{
    const int nrzvals = zrange.nrSteps() + 1;
    const SamplingData<double> sd = getDoubleSamplingData(
						SamplingData<float>( zrange ) );
    if ( veldesc.type_ == VelocityDesc::Avg )
    {
	mAllocVarLenArr( double, zvals, nrzvals );
	if ( !mIsVarLenArrOK(zvals) ) return false;
	for ( int idx=0; idx<nrzvals; idx++ )
	    zvals[idx] = sd.atIndex( idx );

	if ( !computeVint(inpvel,zvals,nrzvals,outvel) )
	    return false;
    }
    else if ( veldesc.type_ == VelocityDesc::RMS &&
	      !computeDix(inpvel,sd,nrzvals,outvel) )
	      return false;
    else if ( veldesc.type_ == VelocityDesc::Interval )
    {
	OD::sysMemCopy( outvel, inpvel, nrzvals*sizeof(float) );
    }

    return true;
}


SamplingData<double> getDoubleSamplingData( const SamplingData<float>& sdf )
{
    SamplingData<double> ret( sdf.start, sdf.step );
    if ( sdf.step > 0.7 )
	return ret;

    float nrsamplesf = 1.f / sdf.step;
    int nrsamples = mNINT32( nrsamplesf );
    float relpos = nrsamplesf - nrsamples;
    if ( Math::Abs(relpos) > nrsamplesf*1e-4f )
	return ret;

    ret.step = 1. / mCast(double,nrsamples);

    nrsamplesf = mCast(float,ret.start / ret.step);
    nrsamples = mNINT32( nrsamplesf );
    relpos = nrsamplesf - nrsamples;
    if ( Math::Abs(relpos) > nrsamplesf*1e-4f )
	return ret;

    ret.start = ret.step * mCast(double, nrsamples);

    return ret;
}
