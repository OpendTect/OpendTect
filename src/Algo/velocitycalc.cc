/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

static const char* rcsID = "$Id: velocitycalc.cc,v 1.3 2008-12-17 23:14:52 cvskris Exp $";

#include "velocitycalc.h"

#include "idxable.h"
#include "math2.h"
#include "valseries.h"
#include "veldesc.h"


TimeDepthConverter::TimeDepthConverter()
    : times_( 0 )
    , depths_( 0 )
    , sz_( 0 )
{}


TimeDepthConverter::~TimeDepthConverter()
{
    delete [] times_;
    delete [] depths_;
}


bool TimeDepthConverter::isOK() const
{ return times_ || depths_; }


bool TimeDepthConverter::setVelocityModel( const ValueSeries<float>& vel,
				      int sz, const SamplingData<double>& sd,
				      const VelocityDesc& vd, bool istime )
{
    delete [] times_; times_ = 0;
    delete [] depths_; depths_ = 0;

    switch ( vd.type_ )
    {
	case VelocityDesc::RMS:
	{
	    if ( !istime ) break;
	    //TODO:: Do dix
	    break;
	}
	case VelocityDesc::Interval:
	{
	    if ( istime )
	    {
		mTryAlloc( depths_, float[sz] );
		if ( !depths_ || !calcDepths(vel,sz,sd,vd.samplespan_,depths_) )
		{ delete [] depths_; depths_ = 0; break; }
	    }
	    else
	    {
		mTryAlloc( times_, float[sz] );
		if ( !times_ || !calcTimes(vel,sz,sd,vd.samplespan_,times_) )
		{ delete [] times_; times_ = 0; break; }
	    }

	    firstvel_ = vel.value(0);
	    lastvel_ = vel.value(sz-1);

	    sz_ = sz;
	    sd_ = sd;
	    break;
	}
    }

    return isOK();
}


bool TimeDepthConverter::calcDepths(ValueSeries<float>& res, int sz,
				    const SamplingData<double>& timesamp) const
{
    if ( !isOK() ) return false;
    if ( depths_ )
    {
	const StepInterval<double> rg( sd_.interval( sz_ ) );
	for ( int idx=0; idx<sz; idx++ )
	{
	    const double time = timesamp.atIndex( idx );

	    float depth;
	    if ( time<=rg.start )
	    {
		const double dt = time-rg.start;
		depth = depths_[0]+dt*firstvel_;
	    }
	    else if ( time>=rg.stop )
	    {
		const double dt = time-rg.stop;
		depth = depths_[sz_-1] + dt*lastvel_;
	    }
	    else
	    {
		depth = IdxAble::interpolateReg(depths_,sz_,rg.getfIndex(time));
	    }

	    res.setValue( idx, depth );
	}
    }
    else
    {
	int timeidx = 0;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const double time = timesamp.atIndex( idx );
	    float depth;
	    if ( time<=times_[0] )
	    {
		const double dt = time-times_[0];
		depth = sd_.start+dt*firstvel_;
	    }
	    else if ( time>times_[sz_-1] )
	    {
		const double dt = time-times_[sz-1];
		depth = sd_.atIndex(sz-1)+dt*lastvel_;
	    }
	    else
	    {
		while ( time>times_[timeidx+1] )
		    timeidx++;

		const float relidx = timeidx +
		    (time-times_[timeidx])/(times_[timeidx+1]-times_[timeidx]);

		depth = sd_.atIndex( relidx );
	    }

	    res.setValue( idx, depth );
	}
    }

    return true;
}


bool TimeDepthConverter::calcTimes( ValueSeries<float>& res, int sz,
				    const SamplingData<double>& depthsamp) const
{
    if ( !isOK() ) return false;
    if ( times_ )
    {
	const StepInterval<double> depthrg( sd_.interval( sz_ ) );
	for ( int idx=0; idx<sz; idx++ )
	{
	    const double depth = depthsamp.atIndex( idx );

	    float time;
	    if ( depth<=depthrg.start )
	    {
		const double ddepth = depth-depthrg.start;
		time = times_[0]+ddepth/firstvel_;
	    }
	    else if ( depth>=depthrg.stop )
	    {
		const double ddepth = depth-depthrg.stop;
		time = times_[sz_-1] + ddepth/lastvel_;
	    }
	    else
	    {
		time = IdxAble::interpolateReg(times_,sz_,
					       depthrg.getfIndex(depth));
	    }

	    res.setValue( idx, time );
	}
    }
    else
    {
	int depthidx = 0;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const double depth = depthsamp.atIndex( idx );
	    float time;
	    if ( depth<=depths_[0] )
	    {
		const double ddepth = depth-depths_[0];
		time = sd_.start+ddepth/firstvel_;
	    }
	    else if ( depth>depths_[sz_-1] )
	    {
		const double ddepth = depth-depths_[sz-1];
		time = sd_.atIndex(sz-1)+ddepth/lastvel_;
	    }
	    else
	    {
		while ( depth>depths_[depthidx+1] )
		    depthidx++;

		const float relidx = depthidx +
		    (depth-depths_[depthidx])/
		    (depths_[depthidx+1]-depths_[depthidx]);

		time = sd_.atIndex( relidx );
	    }

	    res.setValue( idx, time );
	}
    }

    return true;
}



/*!For every time in the velocity model, calculate the depth. The velocity is
   is assumed to be constant at vel[0] above the first time. The sample span
   works as follows:

\code
for sr==above
Time(ms) Vel(m/s) Samplespan (ms)	Depth (m)
20	 1000	  16-20			0.020*1000 = 20			
24	 1500	  20-24			20+0.004*1500 = 26
28	 2000	  24-28			26+0.004*2000 = 34

for sr==center
Time(ms) Vel(m/s) Samplespan (ms)	Depth (m)
20	 1000	  18-22			0.020*1000 = 20			
24	 1500	  22-26			20+0.002*1000+0.002*1500 = 25
28	 2000	  26-30			25+0.002*1500+0.002*2000 = 32

for sr==below
Time(ms) Vel(m/s) Samplespan (ms)	Depth (m)
20	 1000	  20-24			0.020*1000 = 20			
24	 1500	  24-28			20+0.004*1000 = 24
28	 2000	  28-32			24+0.004*1500 = 30
\endcode
*/

bool TimeDepthConverter::calcDepths(const ValueSeries<float>& vels, int velsz,
				    const SamplingData<double>& sd,
				    VelocityDesc::SampleSpan ss,
				    float* depths )
{
    if ( !velsz ) return true;

    float prevvel = vels.value(0);
    double depth = sd.start * prevvel;
    depths[0] = depth;

    for ( int idx=1; idx<velsz; idx++ )
    {
	const float curvel = vels.value( idx );

	if ( ss==VelocityDesc::Above )
	    depth += sd.step*curvel;
	else if ( ss==VelocityDesc::Centered )
	{
	    depth += sd.step * prevvel/2;
	    depth += sd.step * curvel/2;
	}
	else
	    depth += sd.step * prevvel;

	depths[idx] = depth;
	prevvel = curvel;
    }

    return true;
}


/*!For every depth in the velocity model, calculate the time. The velocity is
   is assumed to be constant at vel[0] above the depth time. The sample span
   works as follows:

\code
for sr==above
Depth(m) Vel(m/s) Samplespan (m)	Time (s)
200	 1000	  160-200		200/1000 = 0.2
240	 1500	  200-240		0.2+40/1500 = 0.23
280	 2000	  240-280		0.23+40/2000 = 0.25

for sr==center
Time(ms) Vel(m/s) Samplespan (m)	Time(s)
200	 1000	  180-220		200/1000 = 0.2
240	 1500	  220-260		0.2+20/1000+20/1500=0.23
280	 2000	  260-300		0.23+20/1500+20/2000=0.26

for sr==below
Time(ms) Vel(m/s) Samplespan (m)	Time(s)
200	 1000	  200-240		200/1000 = 0.2
240	 1500	  240-280		0.2+40/1000=0.24
280	 2000	  280-320		0.24+40/1500 = 0.27
\endcode
*/

bool TimeDepthConverter::calcTimes( const ValueSeries<float>& vels, int velsz,
				    const SamplingData<double>& sd,
				    VelocityDesc::SampleSpan ss,
				    float* times )
{
    if ( !velsz ) return true;

    float prevvel = vels.value(0);
    double time = sd.start / prevvel;

    times[0] = time;

    for ( int idx=1; idx<velsz; idx++ )
    {
	const float curvel = vels.value( idx );

	if ( ss==VelocityDesc::Above )
	    time += sd.step/curvel;
	else if ( ss==VelocityDesc::Centered )
	{
	    time += sd.step / prevvel/2;
	    time += sd.step / curvel/2;
	}
	else
	    time += sd.step / prevvel;

	times[idx] = time;
	prevvel = curvel;
    }

    return true;
}


bool computeDix( const float* Vrms, SamplingData<double>& sd, int nrvels,
	                 VelocityDesc::SampleSpan span, float* Vint )
{
    if ( !nrvels )
	return true;

    int idx_prev = -1;
    double v2t_prev = 0;
    double t_above = 0;

    double spanadjustment = 0;
    if ( span==VelocityDesc::Centered )
	spanadjustment = sd.step/2;
    else if ( span==VelocityDesc::Below )
	spanadjustment = sd.step;

    for ( int idx=0; idx<nrvels; idx++ )
    {
	const double v = Vrms[idx];
	if ( mIsUdf(v) )
	    continue;

	double t_below = sd.atIndex( idx ) + spanadjustment;

	const double v2t = t_below*v*v;
	const double numerator = v2t-v2t_prev;
	if ( numerator<0 )
	    continue;

	const double vlayer = Math::Sqrt( numerator/(t_below-t_above) );

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vint[idy] = vlayer;

	v2t_prev = v2t;
	t_above = t_below;
	idx_prev = idx;
    }

    for ( int idx=idx_prev+1; idx<nrvels; idx++ )
	Vint[idx] = Vint[idx_prev];

    return true;
}

