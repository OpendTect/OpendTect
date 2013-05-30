/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "velocitycalc.h"

#include "bendpointfinder.h"
#include "genericnumer.h"
#include "idxable.h"
#include "interpol1d.h"
#include "math2.h"
#include "survinfo.h"
#include "valseries.h"
#include "varlenarray.h"
#include "veldesc.h"
#include "limits.h"

mImplFactory( Vrms2Vint, Vrms2Vint::factory );


TimeDepthModel::TimeDepthModel()
    : errmsg_(0)
    , times_(0)  
    , depths_(0)
    , sz_(0)	
{}


TimeDepthModel::TimeDepthModel( const TimeDepthModel& td )
{
    setModel( td.depths_, td.times_, td.sz_ ); 
}



TimeDepthModel::~TimeDepthModel()
{
    delete [] times_;
    delete [] depths_;
}


bool TimeDepthModel::isOK() const
{ return times_ && depths_ && sz_ >= 0; }


float TimeDepthModel::getDepth( float time ) const
{ return isOK() ? convertTo( depths_, times_, sz_, time, false ) : mUdf(float);}


float TimeDepthModel::getDepth( int idx ) const
{ return isOK() ? depths_[idx] : mUdf(float); }


float TimeDepthModel::getTime( float dpt ) const
{ return isOK() ? convertTo( depths_, times_, sz_, dpt, true ) : mUdf(float); }


float TimeDepthModel::getTime( int idx ) const
{ return isOK() ? times_[idx] : mUdf(float); }


float TimeDepthModel::getLastTime() const
{ return isOK() ? times_[sz_-1] : mUdf(float); }


float TimeDepthModel::getVelocity( float dpt ) const
{ return isOK() ? getVelocity( depths_, times_, sz_, dpt ) : mUdf(float); }


float TimeDepthModel::getDepth( const float* dpths, const float* times,
					int sz, float time) 
{ return convertTo( dpths, times, sz, time, false ); }


float TimeDepthModel::getTime( const float* dpths, const float* times,
					int sz, float dpt) 
{ return convertTo( dpths, times, sz, dpt, true ); }


float TimeDepthModel::getVelocity( const float* dpths, const float* times, 
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
    const float ddt = times[idx1] - times[idx0];
    const float ddh = dpths[idx1] - dpths[idx0];
    return ddt ? ddh / ddt : mUdf(float);
}


float TimeDepthModel::convertTo( const float* dpths, const float* times, int sz, 				 float z, bool targetistime )
{
    const float* zinvals = targetistime ? dpths : times;
    const float* zoutvals = targetistime ? times : dpths;

    int idx1;
    if ( IdxAble::findFPPos( zinvals, sz, z, -1, idx1 ) )
	return zoutvals[idx1];
    else if ( sz < 2 )
	return mUdf(float);
    else if ( idx1 < 0 || idx1 == sz-1 )
    {
	int idx0 = idx1 < 0 ? 1 : idx1;
	const float ddh = dpths[idx0] - dpths[idx0-1];
	const float ddt = times[idx0] - times[idx0-1];
	const float v = ddt ? ddh / ddt : mUdf(float); 
	idx0 = idx1 < 0 ? 0 : idx1;
	const float zshift = z - zinvals[idx0];
	const float zout = zoutvals[idx0]; 
	return targetistime ? ( v ? zout + zshift / v : mUdf(float) )
			    : zout + zshift * v;
    }

    const int idx2 = idx1 + 1;
    const float z1 = z - zinvals[idx1];
    const float z2 = zinvals[idx2] - z;
    return z1+z2 ? (z1 * zoutvals[idx2] + z2 * zoutvals[idx1]) / (z1 + z2) 
		 : mUdf(float);
}


bool TimeDepthModel::setModel( const float* dpths, const float* times, int sz )
{
    mTryAlloc( depths_, float[sz] );
    if ( !depths_ )
	{ errmsg_ = "Out of memory"; return false; }

    mTryAlloc( times_, float[sz] );
    if ( !times_ )
	{ errmsg_ = "Out of memory"; return false; }

    for ( int idx=0; idx<sz; idx++ )
	times_[idx] = times[idx];

    for ( int idx=0; idx<sz; idx++ )
	depths_[idx] = dpths[idx];

    sz_ = sz;

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
					       FixedString* errmsg )
{
    if ( vd.type_==VelocityDesc::Avg || vd.type_==VelocityDesc::Interval )
	return true;

    if ( velintime )
    {
	if ( vd.type_==VelocityDesc::RMS )
	    return true;

	if ( errmsg )
	    *errmsg = "Only RMS, Avg and Interval allowed for time based "
		      "models";
	return false;
    }

    if ( errmsg )
	*errmsg = "Only Avg and Interval allowed for depth based "
		  "models";

    return false;
} 


#define mIsValidVel( v ) (!mIsUdf(v) && v>1e-3)

 
bool TimeDepthConverter::setVelocityModel( const ValueSeries<float>& vel,
				  int sz, const SamplingData<double>& sd,
				  const VelocityDesc& vd, bool istime )
{
    delete [] times_; times_ = 0;
    delete [] depths_; depths_ = 0;

    PtrMan<ValueSeries<float> > ownvint = 0;
    const ValueSeries<float>* vint = &vel;

    switch ( vd.type_ )
    {
	case VelocityDesc::RMS:
	{
	    if ( !istime )
	    {
		errmsg_ = "Cannot use RMS velocities in depth";
		break;
	    }
	    
	    mDeclareAndTryAlloc( float*, ptr, float[sz] );
	    if ( !ptr )
	    {
		errmsg_ = "Out of memory";
		break;
	    }

	    ownvint = new ArrayValueSeries<float,float>( ptr, true, sz );
	    if ( !ownvint || !ownvint->isOK() )
	    {
		errmsg_ = "Out of memory";
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
		errmsg_ = "Out of memory";
		break;
	    }

	    if ( !computeDix( vrms, sd, sz, ownvint->arr() ) )
		break;

	    vint = ownvint.ptr();

	    //Don't break, go into Vint
	}
	case VelocityDesc::Interval:
	{
	    if ( istime )
	    {
		mTryAlloc( depths_, float[sz] );
		if ( !depths_ ||
		     !calcDepths(*vint,sz,sd,depths_) )
		{ delete [] depths_; depths_ = 0; break; }
	    }
	    else
	    {
		mTryAlloc( times_, float[sz] );
		if ( !times_ || !calcTimes(*vint,sz,sd,times_) )
		{ delete [] times_; times_ = 0; break; }
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
		mTryAlloc( depths_, float[sz] );
		if ( !depths_ )
		    break;

		for ( int idx=0; idx<sz; idx++ )
		    depths_[idx] = (float) ( sd.atIndex(idx) * vavg[idx]/2 );
	    }
	    else
	    {
		mTryAlloc( times_, float[sz] );
		if ( !times_ )
		    break;
		 
		for ( int idx=0; idx<sz; idx++ )
		    times_[idx] = (float) ( sd.atIndex(idx) * 2 / vavg[idx] );
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
				    const SamplingData<double>& timesamp) const
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


void TimeDepthConverter::calcZ( const float* zvals, int inpsz, 
			      ValueSeries<float>& res, int outputsz, 
			      const SamplingData<double>& zsamp, bool time)const
{
    float* zrevvals = time ? times_ : depths_;
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
	{
	    zrg = sd_.interval( inpsz );
	}

	for ( int idx=0; idx<outputsz; idx++ )
	{
	    const double z = zsamp.atIndex( idx );
	    float zrev;
	    if ( z <= zrg.start )
	    {
		const double dz = z-zrg.start;
		zrev = (float) ( time ? firstvel_ > 0 ?  
				        (zrevvals[0]+dz*2/firstvel_) 
		    		      : zrevvals[0] 
				      : (zrevvals[0] + dz*firstvel_/2) );
	    }
	    else if ( z >= zrg.stop )
	    {
		const double dz = z-zrg.stop;
		zrev = (float) (  time ?  (zrevvals[inpsz-1] + dz*2/lastvel_) 
				    :  (zrevvals[inpsz-1] + dz*lastvel_/2) );
	    }
	    else
	    {
		float zsample; 
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

		zrev = IdxAble::interpolateReg( zrevvals, inpsz, zsample );
	    }

	    res.setValue( idx, zrev );
	}
    }
    else
    {
	int zidx = 0;
	for ( int idx=0; idx<outputsz; idx++ )
	{
	    const double z = zsamp.atIndex( idx );
	    float zrev;
	    if ( z<=zvals[0] )
	    {
		const double dz = z-zvals[0];
		zrev = (float) ( time ? firstvel_>0 ? (sd_.start+dz*2/firstvel_) 
					  : (sd_.start)
					  : (sd_.start+dz*firstvel_/2) );
	    }
	    else if ( z > zvals[inpsz-1] )
	    {
		const double dz = z-zvals[inpsz-1];
		zrev = (float) ( time ? (sd_.atIndex(inpsz-1)+dz*2/lastvel_)
			    : (sd_.atIndex(inpsz-1)+dz*lastvel_/2) );
	    }
	    else
	    {
		while ( z>zvals[zidx+1] )
		    zidx++;

		const float relidx = (float) ( zidx +
		    (z-zvals[zidx])/(zvals[zidx+1]-zvals[zidx]) );

		zrev = (float) sd_.atIndex( relidx );
	    }

	    res.setValue( idx, zrev );
	}
    }
}



/*!For every time in the velocity model, calculate the depth. The velocity is
   is assumed to be constant at vel[0] above the first time.   The velocity is
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
				     float* depths )
{
    ArrayValueSeries<float,float> times( velsz );
    float* timesptr = times.arr();
    for ( int idx=0; idx<velsz; idx++, timesptr++ )
	*timesptr = (float) ( sd.atIndex( idx ) );

    return calcDepths( vels, velsz, times, depths );
}


bool TimeDepthConverter::calcDepths(const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<float>& times,
				    float* depths )
{
    if ( !depths )
    {
	pFreeFnErrMsg("No output pointer given.",
		      "TimeDepthConverter::calcDepths" );
	return false;
    }
    
    if ( !velsz ) return true;

    float prevvel = mUdf(float);
    int startidx = -1;
    for ( int idx=0; idx<velsz; idx++ )
    {
	if ( !mIsValidVel(vels.value(idx) ) )
	    continue;

	startidx = idx;
	prevvel = vels.value(idx);
	break;
    }

    if ( startidx==-1 )
	return false;

    
    for ( int idx=0; idx<startidx; idx++ )
    {
    	const double depth = times.value(idx) * prevvel / 2;
    	depths[idx] = (float) depth;
    }
    
    double depth = depths[startidx] = times.value(startidx) * prevvel / 2;

    for ( int idx=startidx+1; idx<velsz; idx++ )
    {
	float curvel = vels.value( idx );
	if ( !mIsValidVel(curvel) )
	    curvel = prevvel;

	depth += (times.value(idx)-times.value(idx-1))*curvel/2; //time is TWT

	depths[idx] = (float) depth;
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
				    float* times )
{
    ArrayValueSeries<float,float> depths( velsz );
    float* depthsptr = depths.arr();
    for ( int idx=0; idx<velsz; idx++, depthsptr++ )
	*depthsptr = (float) sd.atIndex( idx );

    return calcTimes( vels, velsz, depths, times );
}


bool TimeDepthConverter::calcTimes( const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<float>& depths,
				    float* times )
{
    if ( !times )
    {
	pFreeFnErrMsg( "No output pointer", "TimeDepthConverter::calcTimes" );
	return false;
    }
    
    if ( !velsz ) return true;

    float prevvel = 0;
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
	float time = depths.value(idx) / prevvel;
	times[idx] = time;
    }

    double time = times[startidx] = depths.value(startidx) / prevvel;

    for ( int idx=startidx+1; idx<velsz; idx++ )
    {
	float curvel = vels.value( idx );
	const double depth = depths.value(idx) - depths.value(idx-1);
	if ( !mIsValidVel(curvel) )
	{
	    curvel = prevvel;
	    if ( curvel>0 )
		time += depth*2/curvel;
	}
	else
    	    time += depth*2/curvel; //time is TWT

	times[idx] = (float) time;
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
    if ( !nrvels ) \
	return true; \
 \
    int idx_prev = -1; \
    double t_above = -first_t; \
    double v2t_prev = t_above*first_v*first_v;; \
    bool hasvals = false; \
 \
    for ( int idx=0; idx<nrvels; idx++ ) \
    { \
	const double v = Vrms[idx]; \
	if ( mIsUdf(v) ) \
	    continue; \
\
	hasvals = true; \
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
	const double vlayer = Math::Sqrt( numerator/(t_below-t_above) ); \
 \
	for ( int idy=idx_prev+1; idy<=idx; idy++ ) \
	    Vint[idy] = (float) vlayer; \
 \
	v2t_prev = v2t; \
	t_above = t_below; \
	idx_prev = idx; \
    } \
 \
    if ( !hasvals ) \
    { \
	for ( int idx=0; idx<nrvels; idx++ ) \
	    Vint[idx] = mUdf(float); \
    } \
 \
    return true

bool computeDix( const float* Vrms, const SamplingData<double>& sd, int nrvels,
		 float* Vint )
{
    mComputeDixImpl( 0, 0, sd.atIndex(idx) );
}


bool computeDix( const float* Vrms, float t0, float v0, const float* t,
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


bool computeVrms( const float* Vint, float t0, const float* t, int nrvels,
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


bool sampleVrms(const float* Vin,float t0_in,float v0_in,const float* t_in,
	int nr_in,const SamplingData<double>& sd_out,float* Vout, int nr_out)
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

    ArrayValueSeries<float,float> tinser ( const_cast<float*>(t_in), false );
    ArrPtrMan<float> deptharr = new float[nr_in];
    ArrPtrMan<float> depthsampled = new float[nr_out];
    ArrPtrMan<float> Vint_sampled = new float[nr_out];
    if ( !tinser.isOK() || !deptharr || !depthsampled || !Vint_sampled )
	return false;

    TimeDepthConverter::calcDepths( Vint, nr_in, tinser, deptharr );
    if ( nr_in<2 )
	return false;
    else
	resampleZ( deptharr, t_in, nr_in, sd_out, nr_out, depthsampled );

    //compute Vint_sampled from depthsampled
    Vint_sampled[0] = 0;
    for ( int idx=1; idx<nr_out; idx++ )
	Vint_sampled[idx] = (float) ( (depthsampled[idx]-depthsampled[idx-1]) /
	    		    (sd_out.step / 2) );		//time is TWT

    return computeVrms( (const float*)Vint_sampled, sd_out, nr_out, Vout );
}


bool computeVavg( const float* Vint, float z0, const float* z, int nrvels,
		  float* Vavg )
{
    double z_above = z0;
    int idx_prev = -1;
    double v2z_prev = 0;

    for ( int idx=0; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double z_below = z[idx];
	float res = 0;
	double dz = z_below - z_above;

	if ( SI().zIsTime() )
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


bool computeVint( const float* Vavg, float z0, const float* z, int nrvels,
		 float* Vint )
{
    int idx_prev = -1; 
    double z_above = z0; 
    double v2z_prev = 0;
    bool hasvals = false; 
 
    for ( int idx=0; idx<nrvels; idx++ ) 
    {
	const double v = Vavg[idx];
	if ( mIsUdf(v) ) 
	    continue;

	hasvals = true;
 
	double z_below = z[idx];
	const bool istime = SI().zIsTime();

	const double v2z = istime ? z_below*v : z_below/v; 
	const double numerator = v2z-v2z_prev; 
	if ( numerator<0 ) 
	    continue; 
 
	if ( ( istime && (z_below<z_above || mIsEqual(z_below,z_above,1e-5)) )
	   || ( !istime && mIsZero(numerator,1e-3) ) ) 
	    continue; 
 
	const double vlayer = istime ? numerator/(z_below-z_above)
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


void resampleZ( const float* zarr, const float* tord_in, int nr_in,
		    const SamplingData<double>& sd_out, int nr_out,
		    float* zsampled )
{
    resampleContinuousData( zarr, tord_in, nr_in, sd_out, nr_out, zsampled );
}


void sampleEffectiveThomsenPars( const float* vinarr, const float* t_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* voutarr )
{
    resampleContinuousData( vinarr, t_in, nr_in, sd_out, nr_out, voutarr );
}


void resampleContinuousData( const float* inarr, const float* t_in, int nr_in,
			const SamplingData<double>& sd_out, int nr_out,
			float* outarr )
{
    int intv = 0;
    const float eps = (float) ( sd_out.step/1e3 );
    for ( int idx=0; idx<nr_out; idx++ )
    {
	bool match = false;
	const float z = (float) sd_out.atIndex( idx );
	for ( ; intv<nr_in; intv++ )
	{
	    if ( mIsEqual( z, t_in[intv], eps ) )
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

	    const float v0 = !intv? 0 : inarr[intv-1];
	    const float v1 = inarr[intv];
	    const float t0 = !intv? 0 : t_in[intv-1];
	    const float t1 = t_in[intv];
	    outarr[idx] = mIsEqual( v0, v1, 1e-6 ) ? v0 :
				Interpolate::linear1D( t0, v0, t1, v1, z );
	}
    }
}


bool sampleVint( const float* Vin,const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out)
{
    if ( !nr_in )
	return false;

    ArrayValueSeries<float,float> tinser ( const_cast<float*>(t_in), false );
    ArrayValueSeries<float,float> Vinser ( const_cast<float*>(Vin), false );
    ArrPtrMan<float> deptharr = new float[nr_in];
    ArrPtrMan<float> depthsampled = new float[nr_out];
    if ( !tinser.isOK() || !Vinser.isOK() || !deptharr || !depthsampled )
	return false;

    TimeDepthConverter::calcDepths( Vinser, nr_in, tinser, deptharr );
    if ( nr_in<2 )
	return false;
    else
	resampleZ( deptharr, t_in, nr_in, sd_out, nr_out, depthsampled );

    //compute Vout from depthsampled
    Vout[0] = Vin[0];
    for ( int idx=1; idx<nr_out; idx++ )
	Vout[idx] = (float) ( (depthsampled[idx] - depthsampled[idx-1]) / 
						   (sd_out.step/2) );
    								//time is TWT
    return true;
}


bool sampleVavg( const float* Vin, const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    ArrPtrMan<float> vintarr = new float[nr_in];
    ArrPtrMan<float> vintsampledarr = new float[nr_out];
    
    if ( !vintarr || !vintsampledarr ) return false;

    TypeSet<float> outtimesamps;
    const float sampstep = (float) sd_out.step;
    const float start = (float) sd_out.start;
    for ( int idx=0; idx<nr_out; idx++ )                            
	outtimesamps += start + idx * sampstep;                   

    return computeVint( Vin, 0, t_in, nr_in, vintarr ) &&
       	sampleVint( vintarr, t_in, nr_in, sd_out, vintsampledarr, nr_out ) &&
	computeVavg( vintsampledarr, start, outtimesamps.arr(), nr_out, Vout); 
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

bool fitLinearVelocity( const float* vint, const float* zin, int nr, 
	const Interval<float>& zlayer, float refz, bool zisdepth, 
	float& v0, float& gradient,  float& error )
{
    if ( nr < 2 )
	return false;

    ArrPtrMan<float> tmp = new float[nr];
    ArrayValueSeries<float,float> inputvels( (float*)vint, false, nr );
    ArrayValueSeries<float,float> inputzs( (float*)zin, false, nr );
    
    if ( zisdepth )
    {
	TimeDepthConverter::calcTimes( inputvels, nr, inputzs, tmp );
    }
    else
    {
    	TimeDepthConverter::calcDepths( inputvels, nr, inputzs, tmp );
    }

    const float* depths = zisdepth ? zin : tmp;
    const float* times = zisdepth ? tmp : zin;

    // Get boundary pairs
    float d[2], t[2], vt[2];
    for ( int idx=0; idx<2; idx++ )
    {
	if ( zisdepth )
	{
	    d[idx] = !idx ? zlayer.start : zlayer.stop;
	    if ( d[idx] <= depths[0] )
	    {
		for ( int idy=0; idy<nr; idy++ )
		{
		    if ( mIsUdf(vint[idy]) ) continue;
	    	    t[idx] = times[idy] - ( depths[idy] - d[idx] ) / vint[idy];
		    vt[idx] = vint[idy];
		    break;
		}
	    }
	    else if ( d[idx] >= depths[nr-1] )
	    {
		for ( int idy=nr-1; idy>=0; idy-- )
		{                   
		    if ( mIsUdf(vint[idy]) ) continue;
		    t[idx] = times[idy] + ( d[idx] - depths[idy] ) / vint[idy];
		    vt[idx] = vint[idy];
		    break;
    		}
	    }
	    else
	    {
		for ( int idy=0; idy<nr-1; idy++ )
		{
		    if ( depths[idy] <= d[idx] && d[idx] <= depths[idy+1] )
		    {
			t[idx] = times[idy] + (times[idy+1] - times[idy]) /
			    (depths[idy+1]-depths[idy]) * (d[idx]-depths[idy]);
			vt[idx] = vint[idy];
			break;
		    }
		}
	    }
	}
	else
	{
	    t[idx] = !idx ? zlayer.start : zlayer.stop;
	    if ( t[idx] <= times[0] )
	    {
		for ( int idy=0; idy<nr; idy++ )
		{
		    if ( mIsUdf(vint[idy]) ) continue;
		    d[idx] = depths[idy] - ( times[idy] - t[idx] ) * vint[idy];
		    vt[idx] = vint[idy];
		    break;
		}
	    }
	    else if ( t[idx] >= times[nr-1] )
	    {
		for ( int idy=nr-1; idy>=0; idy-- )
		{                   
		    if ( mIsUdf(vint[idy]) ) continue;
		    d[idx] = depths[idy] + ( t[idx] - times[idy] ) * vint[idy];
		    vt[idx] = vint[idy];
		    break;
    		}
	    }
	    else
	    {
		for ( int idy=0; idy<nr-1; idy++ )
		{
		    if ( times[idy] <= t[idx] && t[idx] <= times[idy+1] )
		    {
			d[idx] = depths[idy] + (depths[idy+1]-depths[idy]) /
			    (times[idy+1] - times[idy]) * (t[idx] - times[idy]);
			vt[idx] = vint[idy];
			break;
		    }
		}
	    }
	}
    }

    if ( mIsUdf(d[0]) || mIsUdf(d[1]) || mIsUdf(t[0]) || mIsUdf(t[1]) )
	return false;

    const float v = (d[1] - d[0]) / (t[1] - t[0]);
    const float diff = (t[1] + t[0] - 2 * refz) * 0.5f;

    TypeSet<int> indices;
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( zlayer.includes(zin[idx],false) )
	    indices += idx;
    }
    const int nrsmps = indices.size();
    if ( !nrsmps )
    {
	gradient = zisdepth ? (vt[1] - vt[0]) / (d[1] - d[0]) 
	    		    : (vt[1] - vt[0]) / (t[1] - t[0]);
	v0 = zisdepth ? vt[1] - gradient * (d[1] - refz) 
	    	      : vt[1] - gradient * (t[1] - refz);
	return true;
    }

    error = 0;
    if ( zisdepth )
    {
	const StepInterval<float> gradrg(-2, 2, 0.01);//Make your range
	const int nrsteps = (int)(gradrg.width() / gradrg.step);

	const float d10 = d[1] - d[0];
	const float t10 = t[1] - t[0];
	const float d1z = d[1] - refz;
	for ( int idx=0; idx<nrsteps; idx++ )
	{
	    const float grad = gradrg.atIndex( idx );
	    const float v_est = grad * (d10 / (Math::Exp(grad*t10)-1) - d1z);

	    float errsum = 0;
	    for ( int idy=0; idy<nrsmps; idy++ )
	    {
    		const float epsilon = v_est - vint[indices[idy]];
		errsum += epsilon * epsilon;
	    }

	    if ( !idx || error > errsum )
	    {
		error = errsum;
		gradient = grad;
		v0 = v_est;
	    }
	}
    }
    else
    {
	float sum_v0 = 0, sum_grad = 0, sum_vg = 0, sum_v0sqr = 0;
	for ( int idx=0; idx<nrsmps; idx++ )
	{
	    const float grad = (v-(depths[indices[idx]]-d[0]) /
		    (times[indices[idx]]-t[0]))*2 / (t[1]-times[indices[idx]]);
	    const float ve = v - grad * diff;
	    const float epsilon = vint[indices[idx]] - 
		(ve + grad * (times[indices[idx]] - t[0]));
	
	    sum_v0 += ve;
	    sum_grad += grad;
	    sum_vg = ve * grad;
	    sum_v0sqr += ve * ve;
	    error += epsilon * epsilon;
	}

	gradient = nrsmps==1 ? sum_grad :
	    (sum_vg - sum_v0 * sum_grad) / (sum_v0sqr - sum_v0 * sum_v0);
	v0 = v - gradient * diff;
    }

    return true;
}


void sampleIntvThomsenPars( const float* inarr, const float* t_in, int nr_in,
		    const SamplingData<double>& sd_out, int nr_out,
		    float* outarr )
{
    int intv = 0;
    for ( int idx=0; idx<nr_out; idx++ )
    {
	const float z = (float) sd_out.atIndex( idx );
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

