/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitycalc.h"

#include "genericnumer.h"
#include "survinfo.h"
#include "timedepthmodel.h"
#include "veldesc.h"
#include "uistrings.h"
#include "zvalseriesimpl.h"

mImplFactory( Vel::Vrms2Vint, Vel::Vrms2Vint::factory );


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
    float		getValue(float val) const override
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
    const double v2 = (double)Vrms * (double)Vrms;

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


namespace Vel
{

/* Given a discrete table of values xarr, yarr, compute all yvals
   for another set of x positions. This is nothing else than an optimized
   DoubleMathFunction implementing a linear interpolation
 */

static void resampleContinuousData( const ValueSeries<double>& yarr,
				    const ValueSeries<double>& xarr,
				    const ValueSeries<double>& sampledx,
				    double xeps, double yeps,
				    ValueSeries<double>& sampledy )
{
    const od_int64 nr_in = yarr.size();
    const od_int64 nr_out = sampledy.size();
    if ( nr_in == 0 )
	return;
    if ( nr_in == 1 )
    {
	sampledy.setAll( yarr[0] );
	return;
    }

    od_int64 intv = 0;
    for ( od_int64 idx=0; idx<nr_out; idx++ )
    {
	bool match = false;
	const double xval = sampledx[idx];
	for ( ; intv<nr_in; intv++ )
	{
	    if ( mIsEqual(xval,xarr[intv],xeps) )
		match = true;
	    else if ( xarr[intv]<=xval )
		continue;

	    break;
	}

	//intv is always after pos
	if ( match )
	    sampledy.setValue( idx, yarr[intv] );
	else
	{
	    if ( intv == nr_in )
		intv--;

	    const double x0 = intv == 0 ? xarr[intv] : xarr[intv-1];
	    const double y0 = intv == 0 ? yarr[intv] : yarr[intv-1];
	    const double x1 = intv == 0 ? 0. : xarr[intv];
	    const double y1 = intv == 0 ? 0. : yarr[intv];
	    if ( mIsEqual(y0,y1,yeps) )
		sampledy.setValue( idx, y0 );
	    else
		sampledy.setValue( idx, y0 + (xval-x0) * (y1-y0) / (x1-x0) );
	}
    }
}

} // namespace Vel


#define mIsValidVel( v ) (!mIsUdf(v) && v>1e-3)

bool Vel::calcDepthsFromVint( const ValueSeries<double>& Vint,
			      const ZValueSeries& times,
			      ValueSeries<double>& depths )
{
    const od_int64 sz = Vint.size();
    if ( !Vint.isOK() || !times.isOK() || !depths.isOK() ||
	 times.size() != sz || depths.size() != sz )
	return false;

    od_int64 idx_prev;
    for ( idx_prev=0; idx_prev<sz; idx_prev++ )
    {
	if ( mIsValidVel(Vint[idx_prev]) )
	    break;
    }

    if ( idx_prev == sz )
	return false;

    double curvel = Vint[idx_prev];
    for ( od_int64 idx=0; idx<=idx_prev; idx++ )
	depths.setValue( idx, curvel * times[idx] / 2. );

    double curtwt = times[idx_prev];
    double depth = depths[idx_prev];
    for ( od_int64 idx=idx_prev+1; idx<sz; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( !mIsValidVel(V_interval) )
	    continue;

	curvel = V_interval;
	for ( od_int64 idy=idx_prev+1; idy<=idx; idy++ )
	    depths.setValue( idy, depth + curvel * (times[idy]-curtwt) / 2. );

	depth = depths[idx];
	curtwt = times[idx];
	idx_prev = idx;
    }

    for ( od_int64 idy=idx_prev+1; idy<sz; idy++ )
	depths.setValue( idy, depth + curvel * (times[idy]-curtwt) / 2. );

    return true;
}


bool Vel::calcDepthsFromVavg( const ValueSeries<double>& Vavg,
			      const ZValueSeries& times,
			      ValueSeries<double>& depths )
{
    return computeVint( Vavg, times, depths ) &&
	   calcDepthsFromVint( depths, times, depths );
}


bool Vel::calcDepthsFromVrms( const ValueSeries<double>& Vrms,
			      const ZValueSeries& times,
			      ValueSeries<double>& depths, double t0 )
{
    return computeDix( Vrms, times, depths, t0 ) &&
	   calcDepthsFromVint( depths, times, depths );
}


bool Vel::calcTimesFromVint( const ValueSeries<double>& Vint,
			     const ZValueSeries& depths,
			     ValueSeries<double>& times )
{
    const od_int64 sz = Vint.size();
    if ( !Vint.isOK() || !depths.isOK() || !times.isOK() ||
	 depths.size() != sz || times.size() != sz )
	return false;

    od_int64 idx_prev;
    for ( idx_prev=0; idx_prev<sz; idx_prev++ )
    {
	if ( mIsValidVel(Vint[idx_prev]) )
	    break;
    }

    if ( idx_prev == sz )
	return false;

    double curvel = Vint[idx_prev];
    for ( od_int64 idx=0; idx<=idx_prev; idx++ )
	times.setValue( idx, 2. * depths[idx] / curvel );

    double curdepth = depths[idx_prev];
    double time = times[idx_prev];
    for ( od_int64 idx=idx_prev+1; idx<sz; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( !mIsValidVel(V_interval) )
	    continue;

	curvel = V_interval;
	for ( od_int64 idy=idx_prev+1; idy<=idx; idy++ )
	    times.setValue( idy,
			    time + 2. * ( depths[idy] - curdepth ) / curvel );

	time = times[idx];
	curdepth = depths[idx];
	idx_prev = idx;
    }

    for ( od_int64 idy=idx_prev+1; idy<sz; idy++ )
	times.setValue( idy, time + 2. * ( depths[idy] - curdepth ) / curvel );

    return true;
}


bool Vel::calcTimesFromVavg( const ValueSeries<double>& Vavg,
			     const ZValueSeries& depths,
			     ValueSeries<double>& times )
{
    return computeVint( Vavg, depths, times ) &&
	   calcTimesFromVint( times, depths, times );
}


bool Vel::getSampledZ( const ValueSeries<double>& vels,
		       const ZValueSeries& zvals_in, OD::VelocityType type,
		       const ZValueSeries& zvals_out,
		       ValueSeries<double>& Zout, double t0 )
{
    if ( !vels.isOK() || !zvals_in.isOK() || !zvals_out.isOK() || !Zout.isOK()
	 || zvals_in.size() != vels.size()
	 || zvals_out.size() != Zout.size() )
	return false;

    mAllocVarLenArr( double, zarr, vels.size() );
    if ( !mIsVarLenArrOK(zarr) )
	return false;

    const bool velisintime = zvals_in.isTime();
    ArrayZValues rev_zvals_in( zarr, vels.size(),
			       velisintime ? ZDomain::DepthMeter()
					   : ZDomain::TWT() );
    bool res = false;
    if ( velisintime && type == OD::VelocityType::Interval )
	res = calcDepthsFromVint( vels, zvals_in, rev_zvals_in.asVS() );
    if ( velisintime && type == OD::VelocityType::Avg )
	res = calcDepthsFromVavg( vels, zvals_in, rev_zvals_in.asVS() );
    if ( velisintime && type == OD::VelocityType::RMS )
	res = calcDepthsFromVrms( vels, zvals_in, rev_zvals_in.asVS(), t0 );
    if ( !velisintime && type == OD::VelocityType::Interval )
	res = calcTimesFromVint( vels, zvals_in, rev_zvals_in.asVS() );
    if ( !velisintime && type == OD::VelocityType::Avg )
	res = calcTimesFromVavg( vels, zvals_in, rev_zvals_in.asVS() );

    if ( !res )
	return false;

    const ZValueSeries& tarr = velisintime ? zvals_in : rev_zvals_in;
    const ZValueSeries& darr = velisintime ? rev_zvals_in : zvals_in;
    if ( zvals_out.isTime() )
	Vel::resampleContinuousData( darr, tarr, zvals_out, 1e-9, 1e-4, Zout );
    else
	Vel::resampleContinuousData( tarr, darr, zvals_out, 1e-4, 1e-9, Zout );

    return true;
}


bool Vel::calcDepthsFromLinearV0k( double v0, double k,
				   const ZValueSeries& times,
				   ValueSeries<double>& depths )
{
    const od_int64 sz = times.size();
    if ( !times.isOK() || !depths.isOK() || depths.size() != sz ||
	 v0<=0. || mIsUdf(v0) )
	return false;

    const bool zerogradient = mIsZero( k, 1e-6 ) || mIsUdf(k);
    if ( zerogradient )
    {
	if ( sz<=0 )
	    return false;

	double depth = v0 * times[0] / 2.;
	depths.setValue( 0, depth );
	for ( od_int64 idx=1; idx<sz; idx++ )
	{
	    depth += v0 * ( times[idx] - times[idx-1] ) / 2.;
	    depths.setValue( idx, depth );
	}

	return true;
    }

    const double fact = v0/k;
    for ( od_int64 idx=0; idx<sz; idx++ )
    {
	const double time = times[idx];
	const double expterm = Math::Exp( k * time / 2. );
	if ( mIsUdf(expterm) )
	{
	    depths.setValue( idx, mUdf(double) );
	    continue;
	}

	const double depth = fact * ( expterm - 1. );
	if ( depth>MAXDOUBLE )
	    depths.setValue( idx, mUdf(double) );
	else
	    depths.setValue( idx, depth );
    }

    return true;
}


bool Vel::calcTimesFromLinearV0k( double v0, double k,
				  const ZValueSeries& depths,
				  ValueSeries<double>& times )
{
    const od_int64 sz = depths.size();
    if ( !depths.isOK() || !times.isOK() || times.size() != sz ||
	 v0<=0 || mIsUdf(v0) )
	return false;

    const bool zerogradient = mIsZero( k, 1e-6 ) || mIsUdf(k);
    if ( zerogradient )
    {
	if ( sz<=0 )
	    return false;

	double time = 2. * depths[0] / v0;
	times.setValue( 0, time );
	for ( od_int64 idx=1; idx<sz; idx++ )
	{
	    time += 2. * ( depths[idx] - depths[idx-1] ) / v0;
	    times.setValue( idx, time );
	}

	return true;
    }

    const double fact = v0/k;
    for ( od_int64 idx=0; idx<sz; idx++ )
    {
	const double logval = Math::Log( 1. + depths[idx] / fact );
	if ( mIsUdf(logval) )
	    times.setValue( idx, mUdf(double) );
	else
	    times.setValue( idx, 2. * logval/k );
    }

    return true;
}


bool Vel::fitLinearVelocity( const ValueSeries<double>& Vint,
			     const ZValueSeries& zvals,
			     const ::Interval<double>& zlayer, double refz,
			     double& v0, double& gradient, double& error )
{
    const od_int64 nr = Vint.size();
    if ( !Vint.isOK() || !zvals.isOK() || zvals.size() != nr || nr < 2 )
	return false;

    mAllocVarLenArr( double, tmparr, nr );
    if ( !mIsVarLenArrOK(tmparr) )
	return false;

    const bool velisintime = zvals.isTime();
    ArrayZValues rev_zvals( tmparr, nr, velisintime ? ZDomain::DepthMeter()
						    : ZDomain::TWT() );
    if ( zvals.isTime() )
	calcDepthsFromVint( Vint, zvals, rev_zvals.asVS() );
    else
	calcTimesFromVint( Vint, zvals, rev_zvals.asVS() );

    const ValueSeries<double>& times = velisintime ? zvals : rev_zvals;
    const ValueSeries<double>& depths = velisintime ? rev_zvals : zvals;
    const bool zisdepth = zvals.isDepth();

    // Get boundary pairs
    double d[2]{mUdf(double),mUdf(double)};
    double t[2]{mUdf(double),mUdf(double)};
    double vt[2]{mUdf(double),mUdf(double)};
    for ( od_int64 idx=0; idx<2; idx++ )
    {
	if ( zisdepth )
	{
	    d[idx] = !idx ? zlayer.start : zlayer.stop;
	    if ( d[idx] <= depths[0] )
	    {
		for ( od_int64 idy=0; idy<nr; idy++ )
		{
		    const double vel = Vint[idy];
		    if ( mIsUdf(vel) ) continue;
		    t[idx] = times[idy] - ( depths[idy] - d[idx] ) / vel;
		    vt[idx] = vel;
		    break;
		}
	    }
	    else if ( d[idx] >= depths[nr-1] )
	    {
		for ( od_int64 idy=nr-1; idy>=0; idy-- )
		{
		    const double vel = Vint[idy];
		    if ( mIsUdf(vel) ) continue;
		    t[idx] = times[idy] + ( d[idx] - depths[idy] ) / vel;
		    vt[idx] = vel;
		    break;
		}
	    }
	    else
	    {
		for ( od_int64 idy=0; idy<nr-1; idy++ )
		{
		    if ( depths[idy] <= d[idx] && d[idx] <= depths[idy+1] )
		    {
			t[idx] = times[idy] + (times[idy+1] - times[idy]) /
			    (depths[idy+1]-depths[idy]) * (d[idx]-depths[idy]);
			vt[idx] = Vint[idy];
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
		for ( od_int64 idy=0; idy<nr; idy++ )
		{
		    const double vel = Vint[idy];
		    if ( mIsUdf(vel) ) continue;
		    d[idx] = depths[idy] - ( times[idy] - t[idx] ) * vel;
		    vt[idx] = vel;
		    break;
		}
	    }
	    else if ( t[idx] >= times[nr-1] )
	    {
		for ( od_int64 idy=nr-1; idy>=0; idy-- )
		{
		    const double vel = Vint[idy];
		    if ( mIsUdf(vel) ) continue;
		    d[idx] = depths[idy] + ( t[idx] - times[idy] ) * vel;
		    vt[idx] = vel;
		    break;
		}
	    }
	    else
	    {
		for ( od_int64 idy=0; idy<nr-1; idy++ )
		{
		    if ( times[idy] <= t[idx] && t[idx] <= times[idy+1] )
		    {
			d[idx] = depths[idy] + (depths[idy+1]-depths[idy]) /
			    (times[idy+1] - times[idy]) * (t[idx] - times[idy]);
			vt[idx] = Vint[idy];
			break;
		    }
		}
	    }
	}
    }

    if ( mIsUdf(d[0]) || mIsUdf(d[1]) || mIsUdf(t[0]) || mIsUdf(t[1]) )
	return false;

    const double v = (d[1] - d[0]) / (t[1] - t[0]);
    const double diff = (t[1] + t[0] - 2 * refz) * 0.5f;

    TypeSet<od_int64> indices;
    for ( od_int64 idx=0; idx<nr; idx++ )
    {
	if ( zlayer.includes(zvals[idx],false) )
	    indices += idx;
    }
    const od_int64 nrsmps = indices.size();
    if ( !nrsmps )
    {
	gradient = zisdepth ? (vt[1] - vt[0]) / (d[1] - d[0])
			    : (vt[1] - vt[0]) / (t[1] - t[0]);
	v0 = zisdepth ? vt[1] - gradient * (d[1] - refz)
		      : vt[1] - gradient * (t[1] - refz);
	return true;
    }

    error = 0.;
    if ( zisdepth )
    {
	const StepInterval<double> gradrg(-2, 2, 0.01);//Make your range
	const int nrsteps = (int)(gradrg.width() / gradrg.step);

	const double d10 = d[1] - d[0];
	const double t10 = t[1] - t[0];
	const double d1z = d[1] - refz;
	for ( int idx=0; idx<nrsteps; idx++ )
	{
	    const double grad = gradrg.atIndex( idx );
	    const double v_est = grad * (d10 / (Math::Exp(grad*t10)-1) - d1z);

	    double errsum = 0.;
	    for ( od_int64 idy=0; idy<nrsmps; idy++ )
	    {
		const double epsilon = v_est - Vint[indices[idy]];
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
	double sum_v0 = 0., sum_grad = 0., sum_vg = 0., sum_v0sqr = 0.;
	for ( od_int64 idx=0; idx<nrsmps; idx++ )
	{
	    const double grad = (v-(depths[indices[idx]]-d[0]) /
		    (times[indices[idx]]-t[0]))*2 / (t[1]-times[indices[idx]]);
	    const double ve = v - grad * diff;
	    const double epsilon = Vint[indices[idx]] -
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


bool Vel::computeVavg( const ValueSeries<double>& Vint,
		       const ZValueSeries& zvals, ValueSeries<double>& Vavg )
{
    const od_int64 sz = Vint.size();
    if ( !Vint.isOK() || !zvals.isOK() || !Vavg.isOK() ||
	 zvals.size() != sz || Vavg.size() != sz )
	return false;

    ValueSeries<double>& rev_zvals = Vavg;
    if ( (zvals.isTime() && !calcDepthsFromVint(Vint,zvals,rev_zvals)) ||
	 (zvals.isDepth() && !calcTimesFromVint(Vint,zvals,rev_zvals)) )
	return false;

    const ValueSeries<double>& dir_zvals = zvals;
    const ValueSeries<double>& times = zvals.isTime() ? dir_zvals : rev_zvals;
    const ValueSeries<double>& depths = zvals.isDepth() ? dir_zvals : rev_zvals;

    od_int64 startidx = -1;
    for ( od_int64 idx=0; idx<sz; idx++ )
    {
	const double twt = times[idx];
	if ( twt < 1e-2 )
	    continue;
	else if ( startidx < 0 )
	    startidx = idx;

	Vavg.setValue( idx, 2. * depths[idx] / twt );
    }

    if ( startidx >= 0 )
    {
	const double firstvavg = Vavg[startidx];
	for ( od_int64 idx=0; idx<startidx; idx++ )
	    Vavg.setValue( idx, firstvavg );
    }

    return true;
}


bool Vel::computeVrms( const ValueSeries<double>& Vint,
		       const ZValueSeries& tvals, ValueSeries<double>& Vrms,
		       double t0 )
{
    const od_int64 sz = Vint.size();
    if ( !Vint.isOK() || !tvals.isOK() || !Vrms.isOK() || mIsUdf(t0) ||
	 tvals.size() != sz || Vrms.size() != sz || !tvals.isTime() )
	return false;

    od_int64 idx_prev;
    for ( idx_prev=0; idx_prev<sz; idx_prev++ )
    {
	if ( mIsValidVel(Vint[idx_prev]) )
	    break;
    }

    if ( idx_prev == sz )
	return false;

    double curvel = Vint[idx_prev];
    double curtwt;
    double sum_v2t = 0.;
    double prev_twt = t0;
    for ( od_int64 idx=0; idx<=idx_prev; idx++ )
    {
	curtwt = tvals[idx];
	sum_v2t += curvel * curvel * ( curtwt - prev_twt );
	Vrms.setValue( idx, curvel );
	prev_twt = curtwt;
    }

    for ( od_int64 idx=idx_prev+1; idx<sz; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	curvel = V_interval;
	for ( od_int64 idy=idx_prev+1; idy<=idx; idy++ )
	{
	    curtwt = tvals[idy];
	    sum_v2t += curvel * curvel * ( curtwt - prev_twt );
	    Vrms.setValue( idy, Math::Sqrt( sum_v2t / ( curtwt - t0 ) ) );
	    prev_twt = curtwt;
	}

	idx_prev = idx;
    }

    curvel = Vint[idx_prev];
    for ( od_int64 idy=idx_prev+1; idy<sz; idy++ )
    {
	curtwt = tvals[idy];
	sum_v2t += curvel * curvel * ( curtwt - prev_twt );
	Vrms.setValue( idy, Math::Sqrt( sum_v2t / ( curtwt - t0 ) ) );
	prev_twt = curtwt;
    }

    return true;
}


bool Vel::computeVint( const ValueSeries<double>& Vavg,
		       const ZValueSeries& zvals, ValueSeries<double>& Vint )
{
    const od_int64 sz = Vavg.size();
    if ( !Vavg.isOK() || !zvals.isOK() || !Vint.isOK() ||
	 zvals.size() != sz || Vint.size() != sz )
	return false;

    od_int64 idx_prev;
    for ( idx_prev=0; idx_prev<sz; idx_prev++ )
    {
	if ( !mIsUdf(Vavg[idx_prev]) )
	    break;
    }

    if ( idx_prev == sz )
	return false;

    double curvel = Vavg[idx_prev];
    for ( od_int64 idx=0; idx<=idx_prev; idx++ )
	Vint.setValue( idx, curvel );

    const bool zistime = zvals.isTime();
    double z_above = zvals[idx_prev];
    double v2z_prev = zistime ? z_above*curvel : z_above/curvel;
    for ( od_int64 idx=1; idx<sz; idx++ )
    {
	const double V_average = Vavg[idx];
	if ( mIsUdf(V_average) )
	    continue;

	const double z_below = zvals[idx];
	const double v2z = zistime ? z_below*V_average : z_below/V_average;
	const double numerator = v2z-v2z_prev;
	if ( numerator<0 )
	    continue;

	if ( ( zistime && (z_below<z_above || mIsEqual(z_below,z_above,1e-5)) )
	   || ( !zistime && mIsZero(numerator,1e-3) ) )
	    continue;

	curvel = zistime ? numerator/(z_below-z_above)
			 : (z_below-z_above)/numerator;
	for ( od_int64 idy=idx_prev+1; idy<=idx; idy++ )
	    Vint.setValue( idy, curvel );

	v2z_prev = v2z;
	z_above = z_below;
	idx_prev = idx;
    }

    for ( od_int64 idx=idx_prev+1; idx<sz; idx++ )
	Vint.setValue( idx, curvel );

    return true;
}


bool Vel::computeDix( const ValueSeries<double>& Vrms,
		      const ZValueSeries& tvals, ValueSeries<double>& Vint,
		      double t0 )
{
    const od_int64 sz = Vrms.size();
    if ( !Vrms.isOK() || !tvals.isOK() || !Vint.isOK() || mIsUdf(t0) ||
	 tvals.size() != sz || Vint.size() != sz || !tvals.isTime() )
	return false;

    od_int64 idx_prev;
    for ( idx_prev=0; idx_prev<sz; idx_prev++ )
    {
	if ( !mIsUdf(Vrms[idx_prev]) )
	    break;
    }

    if ( idx_prev == sz )
	return false;

    double curvel = Vrms[idx_prev];
    for ( od_int64 idx=0; idx<=idx_prev; idx++ )
	Vint.setValue( idx, curvel );

    double curtwt = tvals[idx_prev];
    double prev_twt = curtwt;
    double v2t_prev = curvel * curvel * ( curtwt - t0 );
    for ( od_int64 idx=idx_prev+1; idx<sz; idx++ )
    {
	const double V_rms = Vrms[idx];
	if ( mIsUdf(V_rms) )
	    continue;

	curtwt = tvals[idx];
	if ( curtwt<prev_twt || mIsEqual(curtwt,prev_twt,1e-8) )
	    continue;

	const double v2t = V_rms * V_rms * ( curtwt - t0 );
	const double numerator = v2t - v2t_prev;
	if ( numerator<0 )
	    continue;

	curvel = Math::Sqrt( numerator/(curtwt-prev_twt) );
	for ( od_int64 idy=idx_prev+1; idy<=idx; idy++ )
	    Vint.setValue( idy, curvel );

	prev_twt = curtwt;
	v2t_prev = v2t;
	idx_prev = idx;
    }

    for ( od_int64 idx=idx_prev+1; idx<sz; idx++ )
	Vint.setValue( idx, curvel );

    return true;
}


bool Vel::sampleVint( const ValueSeries<double>& Vin, const ZValueSeries& z_in,
		      const ZValueSeries& z_out, ValueSeries<double>& Vout )
{
    if ( !Vin.isOK() || !z_in.isOK() || Vin.size() != z_in.size() ||
	 !z_out.isOK() || !Vout.isOK() || Vout.size() != z_out.size() )
	return false;

    Vout.setValue( 0, Vin[0] );
    if ( Vin.size() == 1 )
	return true;

    const od_int64 nr_out = z_out.size();
    mAllocVarLenArr( double, zsampled, nr_out );
    if ( !mIsVarLenArrOK(zsampled) )
	return false;

    ArrayValueSeries<double,double> sampled_zout( zsampled, false, nr_out );
    if ( !getSampledZ(Vin,z_in,OD::VelocityType::Interval,z_out,sampled_zout) )
	return false;

    const bool zistime = z_out.isTime();
    if ( z_out.isRegular() )
    {
	const double& zout_step = dCast(const RegularZValues&,z_out).getStep();
	if ( zistime )
	{
	    const double halfinvstep = 2. / zout_step;
	    for ( od_int64 idx=1; idx<nr_out; idx++ )
	    {
		const double vel =
				halfinvstep * (zsampled[idx] - zsampled[idx-1]);
		Vout.setValue( idx, vel );
	    }
	}
	else
	{
	    const double doublestep = 2. * zout_step;
	    for ( od_int64 idx=1; idx<nr_out; idx++ )
	    {
		const double vel = doublestep / (zsampled[idx]-zsampled[idx-1]);
		Vout.setValue( idx, vel );
	    }
	}
    }
    else
    {
	for ( od_int64 idx=1; idx<nr_out; idx++ )
	{
	    const double dir_dz = z_out[idx] - z_out[idx-1];
	    const double inv_dz = zsampled[idx] - zsampled[idx-1];
	    const double vel = zistime ? 2. * inv_dz / dir_dz
				       : 2. * dir_dz / inv_dz;
	    Vout.setValue( idx, vel );
	}
    }

    return true;
}


bool Vel::sampleVavg( const ValueSeries<double>& Vin,
		      const ZValueSeries& z_in, const ZValueSeries& z_out,
		      ValueSeries<double>& Vout )
{
    if ( !Vin.isOK() || !z_in.isOK() || Vin.size() != z_in.size() ||
	 !z_out.isOK() || !Vout.isOK() || Vout.size() != z_out.size() )
	return false;

    const od_int64 nr_out = z_out.size();
    mAllocVarLenArr( double, zsampled, nr_out );
    if ( !mIsVarLenArrOK(zsampled) )
	return false;

    ArrayValueSeries<double,double> sampled_zout( zsampled, false, nr_out );
    if ( !getSampledZ(Vin,z_in,OD::VelocityType::Avg,z_out,sampled_zout) )
	return false;

    od_int64 firstvalid  = -1;
    const ValueSeries<double>& dir_zvals = z_out;
    const ValueSeries<double>& sampled_zvals = sampled_zout;
    const ValueSeries<double>& times = z_out.isTime() ? dir_zvals
						      : sampled_zvals;
    const ValueSeries<double>& depths = z_out.isDepth() ? dir_zvals
							: sampled_zvals;
    for ( od_int64 idx=0; idx<nr_out; idx++ )
    {
	const double twt = times[idx];
	if ( mIsZero(twt,1e-9) )
	    continue;
	else if ( firstvalid < 0 )
	    firstvalid = idx;
	Vout.setValue( idx, 2. * depths[idx] / twt );
    }

    if ( firstvalid >=0 )
    {
	const double firstvel = Vout[firstvalid];
	for ( od_int64 idx=0; idx<firstvalid; idx++ )
	    Vout.setValue( idx, firstvel );
    }

    return true;
}


bool Vel::sampleVrms( const ValueSeries<double>& Vin,
		      const ZValueSeries& t_in, const ZValueSeries& t_out,
		      ValueSeries<double>& Vout, double t0_in )
{
    const od_int64 nr_in = Vin.size();
    const od_int64 nr_out = Vout.size();
    if ( !Vin.isOK() || !t_in.isOK() || t_in.size() != nr_in ||
	 !t_out.isOK() || !Vout.isOK() || t_out.size() != nr_out ||
	 !t_in.isTime() || !t_out.isTime() )
	return false;

    mAllocVarLenArr( double, Vint, nr_in );
    mAllocVarLenArr( double, Vint_sampled, nr_out );
    if ( !mIsVarLenArrOK(Vint) || !mIsVarLenArrOK(Vint_sampled) )
	return false;

    ArrayZValues Vintarr( Vint, nr_in, ZDomain::TWT() );
    ArrayZValues Vintsampledarr( Vint_sampled, nr_out, ZDomain::TWT() );

    return computeDix( Vin, t_in, Vintarr.asVS(), t0_in ) &&
	   sampleVint( Vintarr.asVS(), t_in, t_out, Vintsampledarr.asVS() ) &&
	   computeVrms( Vintsampledarr.asVS(), t_out, Vout, t0_in );
}


void Vel::sampleEffectiveThomsenPars( const ValueSeries<double>& inparr,
				      const ZValueSeries& z_in,
				      const ZValueSeries& z_out,
				      ValueSeries<double>& res )
{
    const double xeps = z_out.isTime() ? 1e-9 : 1e-4;
    Vel::resampleContinuousData( inparr, z_in, z_out, xeps, 1e-6, res );
}


void Vel::sampleIntvThomsenPars( const ValueSeries<double>& inparr,
				 const ZValueSeries& z_in,
				 const ZValueSeries& z_out,
				 ValueSeries<double>& res )
{
    const od_int64 nr_in = inparr.size();
    const od_int64 nr_out = res.size();
    if ( !inparr.isOK() || !z_in.isOK() || !z_out.isOK() || !res.isOK() ||
	 z_in.size() != nr_in || z_out.size() != nr_out )
	return;

    od_int64 intv = 0;
    for ( od_int64 idx=0; idx<nr_out; idx++ )
    {
	const double z = z_out[idx];
	for ( ; intv<nr_in; intv++ )
	{
	    if ( z_in[intv]<z )
		continue;

	    break;
	}

	//intv is always after pos
	if ( intv == nr_in )
	    intv--;

	res.setValue( idx, inparr[intv] );
    }
}


void Vel::computeResidualMoveouts( double z0, double rmo, double refoffset,
				   int nroffsets, bool outputdepth,
				   const double* offsets, double* output )
{
    const bool alludf =
	    (outputdepth && mIsUdf(z0)) || mIsUdf(rmo) || mIsUdf(refoffset) ||
	    mIsZero(refoffset,1e-5);

    for ( int idx=0; idx<nroffsets; idx++, offsets++, output++ )
    {
	if ( alludf )
	{
	    *output = mUdf(double);
	    continue;
	}

	const double offset = *offsets;
	if ( mIsUdf(offset) )
	{
	    *output = mUdf(double);
	    continue;
	}

	const double ratio = offset/refoffset;
	*output = rmo * ratio * ratio;
	if ( outputdepth )
	    *output += z0;
    }
}


// DixConversion

bool Vel::DixConversion::compute( const ValueSeries<double>& Vrms,
				  const ZValueSeries& tvals,
				  ValueSeries<double>& Vint, double t0 )
{
    return computeDix( Vrms, tvals, Vint, t0 );
}


mStartAllowDeprecatedSection

mImplFactory( Vrms2Vint, Vrms2Vint::factory );


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


bool computeDix( const float* Vrms, float t0, float v0, const float* t,
		 int nrvels, float* Vint )
{
    mAllocVarLenArr( double, tvals, nrvels );
    if ( !mIsVarLenArrOK(tvals) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	tvals[idx] = t[idx];

    return computeDix( Vrms, (double)t0, v0, tvals, nrvels, Vint );
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


bool computeVrms( const float* Vint, float t0, const float* t, int nrvels,
		  float* Vrms )
{
    mAllocVarLenArr( double, td_in, nrvels );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	td_in[idx] = t[idx];

    return computeVrms( Vint, (double)t0, td_in, nrvels, Vrms );
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


bool sampleVrms( const float* Vin, float t0_in, float v0_in, const float* t_in,
		 int nr_in, const SamplingData<double>& sd_out, float* Vout,
		 int nr_out )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    return sampleVrms( Vin, (double)t0_in, v0_in, td_in, nr_in, sd_out, Vout,
		       nr_out );
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


bool computeVavg( const float* Vint, float z0, const float* z, int nrvels,
		  float* Vavg )
{
    mAllocVarLenArr( double, zvals, nrvels );
    if ( !mIsVarLenArrOK(zvals) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	zvals[idx] = z[idx];

    return computeVavg( Vint, zvals, nrvels, Vavg );
}


bool computeVavg( const float* Vint, const double* z, int nrvels, float* Vavg )
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


bool computeVint( const float* Vavg, float z0, const float* z, int nrvels,
		  float* Vint )
{
    mAllocVarLenArr( double, zvals, nrvels );
    if ( !mIsVarLenArrOK(zvals) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	zvals[idx] = z[idx];

    return computeVint( Vavg, zvals, nrvels, Vint );
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


void resampleZ( const float* zarr, const float* tord_in, int nr_in,
		const SamplingData<double>& sd_out, int nr_out,float* zsampled )
{
    mAllocVarLenArr( double, dzarr, nr_in );
    mAllocVarLenArr( double, tordd_in, nr_in );
    mAllocVarLenArr( double, dzsampled, nr_out );
    if ( !mIsVarLenArrOK(dzarr) || !mIsVarLenArrOK(tordd_in) ||
	 !mIsVarLenArrOK(dzsampled) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
    {
	dzarr[idx] = zarr[idx];
	tordd_in[idx] = tord_in[idx];
    }

    resampleContinuousData( dzarr, tordd_in, nr_in, sd_out, nr_out, dzsampled );
    for ( int idx=0; idx<nr_out; idx++ )
	zsampled[idx] = mCast(float,zsampled[idx]);
}


void resampleZ( const double* zarr, const double* tord_in, int nr_in,
		const SamplingData<double>& sd_out, int nr_out,double* zsampled)
{
    resampleContinuousData( zarr, tord_in, nr_in, sd_out, nr_out, zsampled );
}


void sampleEffectiveThomsenPars( const float* vinarr, const float* t_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* voutarr )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    sampleEffectiveThomsenPars( vinarr, td_in, nr_in, sd_out, nr_out, voutarr );
}


void sampleEffectiveThomsenPars( const float* vinarr, const double* t_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* voutarr )
{
    mAllocVarLenArr( double, dvinarr, nr_in );
    mAllocVarLenArr( double, dvoutarr, nr_out );
    if ( !mIsVarLenArrOK(dvinarr) || !mIsVarLenArrOK(dvoutarr) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
	dvinarr[idx] = vinarr[idx];

    resampleContinuousData( dvinarr, t_in, nr_in, sd_out, nr_out, dvoutarr );
    for ( int idx=0; idx<nr_out; idx++ )
	voutarr[idx] = mCast(float,dvoutarr[idx]);
}


void resampleContinuousData( const float* inarr, const float* t_in, int nr_in,
			     const SamplingData<double>& sd_out, int nr_out,
			     float* outarr )
{
    mAllocVarLenArr( double, dinarr, nr_in );
    mAllocVarLenArr( double, td_in, nr_in );
    mAllocVarLenArr( double, doutarr, nr_out );
    if ( !mIsVarLenArrOK(dinarr) || !mIsVarLenArrOK(td_in) ||
	 !mIsVarLenArrOK(doutarr) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
    {
	dinarr[idx] = inarr[idx];
	td_in[idx] = t_in[idx];
    }

    resampleContinuousData( dinarr, td_in, nr_in, sd_out, nr_out, doutarr );
    for ( int idx=0; idx<nr_out; idx++ )
	outarr[idx] = mCast(float,doutarr[idx]);
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


bool sampleVint( const float* Vin, const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    return sampleVint( Vin, td_in, nr_in, sd_out, Vout, nr_out );
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


bool sampleVavg( const float* Vin, const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    return sampleVavg( Vin, td_in, nr_in, sd_out, Vout, nr_out );
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

bool fitLinearVelocity( const float* vint, const float* zin, int nr,
	const Interval<float>& zlayer, float refz, bool zisdepth,
	float& v0, float& gradient,  float& error )
{
    if ( nr < 2 )
	return false;

    mAllocVarLenArr( float, tmparr, nr );
    if ( !mIsVarLenArrOK(tmparr) ) return false;
    const ArrayValueSeries<float,float> inputvels( (float*)vint, false, nr );
    const ArrayValueSeries<float,float> inputzs( (float*)zin, false, nr );

    if ( zisdepth )
    {
	TimeDepthConverter::calcTimes( inputvels, nr, inputzs, tmparr );
    }
    else
    {
	TimeDepthConverter::calcDepths( inputvels, nr, inputzs, tmparr );
    }

    const float* depths = zisdepth ? zin : tmparr;
    const float* times = zisdepth ? tmparr : zin;

    // Get boundary pairs
    float d[2]{mUdf(float),mUdf(float)};
    float t[2]{mUdf(float),mUdf(float)};
    float vt[2]{mUdf(float),mUdf(float)};
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
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    sampleIntvThomsenPars( inarr, td_in, nr_in, sd_out, nr_out, outarr );
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

mStopAllowDeprecatedSection
