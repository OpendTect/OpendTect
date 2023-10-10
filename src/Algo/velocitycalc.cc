/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitycalc.h"

#include "genericnumer.h"
#include "survinfo.h"
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
	    res[idx] = float (Math::Sqrt( t2 ));
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
			      const ZValueSerie& times,
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
			      const ZValueSerie& times,
			      ValueSeries<double>& depths )
{
    return computeVint( Vavg, times, depths ) &&
	   calcDepthsFromVint( depths, times, depths );
}


bool Vel::calcDepthsFromVrms( const ValueSeries<double>& Vrms,
			      const ZValueSerie& times,
			      ValueSeries<double>& depths, double t0 )
{
    return computeDix( Vrms, times, depths, t0 ) &&
	   calcDepthsFromVint( depths, times, depths );
}


bool Vel::calcTimesFromVint( const ValueSeries<double>& Vint,
			     const ZValueSerie& depths,
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
			     const ZValueSerie& depths,
			     ValueSeries<double>& times )
{
    return computeVint( Vavg, depths, times ) &&
	   calcTimesFromVint( times, depths, times );
}


bool Vel::getSampledZ( const ValueSeries<double>& vels,
		       const ZValueSerie& zvals_in, Vel::Type type,
		       const ZValueSerie& zvals_out,
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
    if ( velisintime && type == Vel::Interval )
	res = calcDepthsFromVint( vels, zvals_in, rev_zvals_in );
    if ( velisintime && type == Vel::Avg )
	res = calcDepthsFromVavg( vels, zvals_in, rev_zvals_in );
    if ( velisintime && type == Vel::RMS )
	res = calcDepthsFromVrms( vels, zvals_in, rev_zvals_in, t0 );
    if ( !velisintime && type == Vel::Interval )
	res = calcTimesFromVint( vels, zvals_in, rev_zvals_in );
    if ( !velisintime && type == Vel::Avg )
	res = calcTimesFromVavg( vels, zvals_in, rev_zvals_in );

    if ( !res )
	return false;

    const ZValueSerie& tarr = velisintime ? zvals_in : rev_zvals_in;
    const ZValueSerie& darr = velisintime ? rev_zvals_in : zvals_in;
    if ( zvals_out.isTime() )
	Vel::resampleContinuousData( darr, tarr, zvals_out, 1e-9, 1e-4, Zout );
    else
	Vel::resampleContinuousData( tarr, darr, zvals_out, 1e-4, 1e-9, Zout );

    return true;
}


bool Vel::calcDepthsFromLinearV0k( double v0, double k,
				   const ZValueSerie& times,
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
				  const ZValueSerie& depths,
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
			     const ZValueSerie& zvals,
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
	calcDepthsFromVint( Vint, zvals, rev_zvals );
    else
	calcTimesFromVint( Vint, zvals, rev_zvals );

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
		       const ZValueSerie& zvals, ValueSeries<double>& Vavg )
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
		       const ZValueSerie& tvals, ValueSeries<double>& Vrms,
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
		       const ZValueSerie& zvals, ValueSeries<double>& Vint )
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
		      const ZValueSerie& tvals, ValueSeries<double>& Vint,
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


bool Vel::sampleVint( const ValueSeries<double>& Vin, const ZValueSerie& z_in,
		      const ZValueSerie& z_out, ValueSeries<double>& Vout )
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
    if ( !getSampledZ(Vin,z_in,Vel::Interval,z_out,sampled_zout) )
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
		      const ZValueSerie& z_in, const ZValueSerie& z_out,
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
    if ( !getSampledZ(Vin,z_in,Vel::Avg,z_out,sampled_zout) )
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
		      const ZValueSerie& t_in, const ZValueSerie& t_out,
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

    return computeDix( Vin, t_in, Vintarr, t0_in ) &&
	   sampleVint( Vintarr, t_in, t_out, Vintsampledarr ) &&
	   computeVrms( Vintsampledarr, t_out, Vout, t0_in );
}


void Vel::sampleEffectiveThomsenPars( const ValueSeries<double>& inparr,
				      const ZValueSerie& z_in,
				      const ZValueSerie& z_out,
				      ValueSeries<double>& res )
{
    const double xeps = z_out.isTime() ? 1e-9 : 1e-4;
    Vel::resampleContinuousData( inparr, z_in, z_out, xeps, 1e-6, res );
}


void Vel::sampleIntvThomsenPars( const ValueSeries<double>& inparr,
				 const ZValueSerie& z_in,
				 const ZValueSerie& z_out,
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
				  const ZValueSerie& tvals,
				  ValueSeries<double>& Vint, double t0 )
{
    return computeDix( Vrms, tvals, Vint, t0 );
}


// Deprecated implementations:

mImplFactory( Vrms2Vint, Vrms2Vint::factory );

SamplingData<double> getDoubleSamplingData( const SamplingData<float>& sdf )
{
    return RegularZValues::getDoubleSamplingData( sdf );
}


bool computeLinearT2D( double v0, double dv, double v0depth,
		       const SamplingData<float>& sd_in, int sz, float* res )
{
    const RegularZValues tvals( sd_in, sz, ZDomain::TWT() );
    ArrayValueSeries<double,float> depths( res, false, sz );
    if ( !Vel::calcDepthsFromLinearV0k(v0,dv,tvals,depths) )
	return false;

    for ( int idx=0; idx<sz; idx++ )
	res[idx] -= v0depth;

    return true;
}


bool computeLinearD2T( double v0, double dv, double v0depth,
		       const SamplingData<float>& sd_in, int sz, float* res )
{
    SamplingData<float> sd( sd_in );
    sd.start -= v0depth;
    const RegularZValues dvals( sd, sz, SI().zInFeet() ? ZDomain::DepthFeet()
						       : ZDomain::DepthMeter());
    ArrayValueSeries<double,float> times( res, false, sz );
    return Vel::calcTimesFromLinearV0k( v0, dv, dvals, times );
}


bool fitLinearVelocity( const float* vint, const float* zin, int nr,
			     const Interval<float>& zlayer_in, float refz,
			     bool zisdepth, float& v0_in, float& gradient_in,
			     float& error_in )
{
    const ArrayValueSeries<double,float> Vin( (float*)(vint), false, nr );
    const ZDomain::Info& zinfo = zisdepth ? (SI().zInFeet()
						? ZDomain::DepthFeet()
						: ZDomain::DepthMeter())
					  : ZDomain::TWT();
    const ArrayZValues<float> zvals( (float*)(zin), nr, zinfo );
    const ::Interval<double> zlayer( zlayer_in.start, zlayer_in.stop );
    const double reference_z = refz;
    double v0, gradient, error;
    if ( !Vel::fitLinearVelocity(Vin,zvals,zlayer,reference_z,
				 v0,gradient,error) )
	return false;

    v0_in = float (v0);
    gradient_in = float (gradient);
    error_in = float (error);

    return true;
}


bool convertToVintIfNeeded( const float*, const VelocityDesc&,
			    const ZSampling&, float* )
{
    pFreeFnErrMsg("Use function from Vel namespace");
    return false;
}


bool computeVavg( const float* Vint_in, const double* z_in,
		  int nrvels, float* Vavg_in )
{
    const ArrayValueSeries<double,float> Vint( (float*)(Vint_in),false,nrvels);
    const ArrayZValues<double> zvals( (double*)(z_in), nrvels,
				      SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vavg( Vavg_in, false, nrvels );
    return Vel::computeVavg( Vint, zvals, Vavg );
}


bool computeVavg( const float* Vint_in, float /* z0 */, const float* z_in,
		  int nrvels, float* Vavg_in )
{
    const ArrayValueSeries<double,float> Vint( (float*)(Vint_in),false,nrvels);
    const ArrayZValues<float> zvals( (float*)(z_in), nrvels,
				     SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vavg( Vavg_in, false, nrvels );
    return Vel::computeVavg( Vint, zvals, Vavg );
}


bool computeVrms( const float* Vint_in, const SamplingData<double>& sd,
		  int nrvels, float* Vrms_in )
{
    const ArrayValueSeries<double,float> Vint( (float*)(Vint_in),false,nrvels);
    const RegularZValues zvals( sd, nrvels, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vrms( Vrms_in, false, nrvels );
    return Vel::computeVrms( Vint, zvals, Vrms );
}


bool computeVrms( const float* Vint_in, double t0, const double* t_in,
		  int nrvels, float* Vrms_in )
{
    const ArrayValueSeries<double,float> Vint( (float*)(Vint_in),false,nrvels);
    const ArrayZValues<double> zvals( (double*)(t_in), nrvels, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vrms( Vrms_in, false, nrvels );
    return Vel::computeVrms( Vint, zvals, Vrms, t0 );
}


bool computeVrms( const float* Vint_in, float t0, const float* t_in,
		  int nrvels, float* Vrms_in )
{
    const ArrayValueSeries<double,float> Vint( (float*)(Vint_in),false,nrvels);
    const ArrayZValues<float> zvals( (float*)(t_in), nrvels, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vrms( Vrms_in, false, nrvels );
    return Vel::computeVrms( Vint, zvals, Vrms, t0 );
}


bool computeVint( const float* Vavg_in, const double* z_in, int nrvels,
		  float* Vint_in )
{
    const ArrayValueSeries<double,float> Vavg( (float*)(Vavg_in),false,nrvels);
    const ArrayZValues<double> zvals( (double*)(z_in), nrvels,
				      SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vint( Vint_in, false, nrvels );
    return Vel::computeVint( Vavg, zvals, Vint );
}


bool computeVint( const float* Vavg_in, float /* z0 */, const float* z_in,
		  int nrvels, float* Vint_in )
{
    const ArrayValueSeries<double,float> Vavg( (float*)(Vavg_in),false,nrvels);
    const ArrayZValues<float> zvals( (float*)(z_in), nrvels,
				     SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vint( Vint_in, false, nrvels );
    return Vel::computeVint( Vavg, zvals, Vint );

}


bool computeDix( const float* Vrms_in, const SamplingData<double>& sd,
		 int nrvels, float* Vint_in )
{
    const ArrayValueSeries<double,float> Vrms( (float*)(Vrms_in),false,nrvels);
    const RegularZValues zvals( sd, nrvels, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vint( Vint_in, false, nrvels );
    return Vel::computeDix( Vrms, zvals, Vint );
}


bool computeDix( const float* Vrms_in, double t0, float /* v0 */,
		 const double* t_in, int nrvels, float* Vint_in )
{
    const ArrayValueSeries<double,float> Vrms( (float*)(Vrms_in),false,nrvels);
    const ArrayZValues<double> zvals( (double*)(t_in), nrvels, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vint( Vint_in, false, nrvels );
    return Vel::computeDix( Vrms, zvals, Vint, t0 );
}


bool computeDix( const float* Vrms_in, float t0, float /* v0 */,
		 const float* t_in, int nrvels, float* Vint_in )
{
    const ArrayValueSeries<double,float> Vrms( (float*)(Vrms_in),false,nrvels);
    const ArrayZValues<float> zvals( (float*)(t_in), nrvels, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vint( Vint_in, false, nrvels );
    return Vel::computeDix( Vrms, zvals, Vint, t0 );
}


bool sampleVint( const float* Vin_in, const double* z_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout_in, int nr_out)
{
    const ArrayValueSeries<double,float> Vin( (float*)(Vin_in), false, nr_in );
    const ArrayZValues<double> zvals_in( (double*)(z_in), nr_in,
					 SI().zDomainInfo() );
    const RegularZValues zvals_out( sd_out, nr_out,
				    SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vout( Vout_in, false, nr_out );
    return Vel::sampleVint( Vin, zvals_in, zvals_out, Vout );
}


bool sampleVint( const float* Vin_in, const float* z_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout_in, int nr_out)
{
    const ArrayValueSeries<double,float> Vin( (float*)(Vin_in), false, nr_in );
    const ArrayZValues<float> zvals_in( (float*)(z_in), nr_in,
					SI().zDomainInfo() );
    const RegularZValues zvals_out( sd_out, nr_out,
				    SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vout( Vout_in, false, nr_out );
    return Vel::sampleVint( Vin, zvals_in, zvals_out, Vout );
}


bool sampleVavg( const float* Vin_in, const double* z_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout_in, int nr_out)
{
    const ArrayValueSeries<double,float> Vin( (float*)(Vin_in), false, nr_in );
    const ArrayZValues<double> zvals_in( (double*)(z_in), nr_in,
					 SI().zDomainInfo() );
    const RegularZValues zvals_out( sd_out, nr_out,
				    SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vout( Vout_in, false, nr_out );
    return Vel::sampleVavg( Vin, zvals_in, zvals_out, Vout );
}


bool sampleVavg( const float* Vin_in, const float* z_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout_in, int nr_out)
{
    const ArrayValueSeries<double,float> Vin( (float*)(Vin_in), false, nr_in );
    const ArrayZValues<float> zvals_in( (float*)(z_in), nr_in,
					SI().zDomainInfo() );
    const RegularZValues zvals_out( sd_out, nr_out,
				    SI().zDomainInfo() );
    ArrayValueSeries<double,float> Vout( Vout_in, false, nr_out );
    return Vel::sampleVavg( Vin, zvals_in, zvals_out, Vout );
}


bool sampleVrms( const float* Vin_in, double t0_in, float /* v0_in */,
		 const double* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout_in, int nr_out)
{
    const ArrayValueSeries<double,float> Vin( (float*)(Vin_in), false, nr_in );
    const ArrayZValues<double> zvals_in( (double*)(t_in), nr_in,ZDomain::TWT());
    const RegularZValues zvals_out( sd_out, nr_out, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vout( Vout_in, false, nr_out );
    return Vel::sampleVrms( Vin, zvals_in, zvals_out, Vout, t0_in );
}


bool sampleVrms( const float* Vin_in, float t0_in, float /* v0_in */,
		 const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout_in, int nr_out)
{
    const ArrayValueSeries<double,float> Vin( (float*)(Vin_in), false, nr_in );
    const ArrayZValues<float> zvals_in( (float*)(t_in), nr_in, ZDomain::TWT() );
    const RegularZValues zvals_out( sd_out, nr_out, ZDomain::TWT() );
    ArrayValueSeries<double,float> Vout( Vout_in, false, nr_out );
    return Vel::sampleVrms( Vin, zvals_in, zvals_out, Vout, double(t0_in) );
}


void sampleEffectiveThomsenPars( const float* inparr, const float* z_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* resarr )
{
    const ArrayValueSeries<double,float> inp( (float*)(inparr), false, nr_in );
    const ArrayZValues<float> zvals_in( (float*)(z_in), nr_in,
					SI().zDomainInfo() );
    const RegularZValues zvals_out( sd_out, nr_out, SI().zDomainInfo() );
    ArrayValueSeries<double,float> res( resarr, false, nr_out );
    Vel::sampleEffectiveThomsenPars( inp, zvals_in, zvals_out, res );
}


void sampleIntvThomsenPars( const float* inparr, const float* z_in, int nr_in,
			    const SamplingData<double>& sd_out, int nr_out,
			    float* resarr )
{
    const ArrayValueSeries<double,float> inp( (float*)(inparr), false, nr_in );
    const ArrayZValues<float> zvals_in( (float*)(z_in), nr_in,
					SI().zDomainInfo() );
    const RegularZValues zvals_out( sd_out, nr_out, SI().zDomainInfo() );
    ArrayValueSeries<double,float> res( resarr, false, nr_out );
    Vel::sampleIntvThomsenPars( inp, zvals_in, zvals_out, res );
}


void resampleZ( const double* zarr, const double* tord_in, int nr_in,
		const SamplingData<double>& sd_out, int nr_out,double* zsampled)
{
    const ArrayValueSeries<double,float> yarr( (float*)(zarr), false, nr_in );
    const ArrayValueSeries<double,float> xarr( (float*)(tord_in), false, nr_in);
    const RegularZValues zvals_out( sd_out, nr_out, SI().zDomainInfo() );
    ArrayValueSeries<double,double> res( zsampled, false, nr_out );
    const double xeps = SI().zIsTime() ? 1e-4 : 1e-9;
    const double yeps = SI().zIsTime() ? 1e-9 : 1e-4;
    Vel::resampleContinuousData( yarr, xarr, zvals_out, xeps, yeps, res );
}


void resampleZ( const float* zarr, const float* tord_in, int nr_in,
		const SamplingData<double>& sd_out, int nr_out,float* zsampled )
{
    const ArrayValueSeries<double,float> yarr( (float*)(zarr), false, nr_in );
    const ArrayValueSeries<double,float> xarr( (float*)(tord_in), false, nr_in);
    const RegularZValues zvals_out( sd_out, nr_out, SI().zDomainInfo() );
    ArrayValueSeries<double,float> res( zsampled, false, nr_out );
    const double xeps = SI().zIsTime() ? 1e-4 : 1e-9;
    const double yeps = SI().zIsTime() ? 1e-9 : 1e-4;
    Vel::resampleContinuousData( yarr, xarr, zvals_out, xeps, yeps, res );
}


void resampleContinuousData( const double* inparr, const double* tord_in,
			     int nr_in, const SamplingData<double>& sd_out,
			     int nr_out, double* outarr )
{
    const ArrayValueSeries<double,double> yarr( (double*)(inparr), false,nr_in);
    const ArrayValueSeries<double,double> xarr( (double*)(tord_in),false,nr_in);
    const RegularZValues zvals_out( sd_out, nr_out,SI().zDomainInfo() );
    ArrayValueSeries<double,double> res( outarr, false, nr_out );
    const double xeps = SI().zIsTime() ? 1e-4 : 1e-9;
    const double yeps = SI().zIsTime() ? 1e-9 : 1e-4;
    Vel::resampleContinuousData( yarr, xarr, zvals_out, xeps, yeps, res );
}


void resampleContinuousData( const float* inparr, const float* tord_in,
			     int nr_in, const SamplingData<double>& sd_out,
			     int nr_out, float* outarr )
{
    const ArrayValueSeries<double,float> yarr( (float*)(inparr), false, nr_in );
    const ArrayValueSeries<double,float> xarr( (float*)(tord_in), false, nr_in);
    const RegularZValues zvals_out( sd_out, nr_out, SI().zDomainInfo() );
    ArrayValueSeries<double,float> res( outarr, false, nr_out );
    const double xeps = SI().zIsTime() ? 1e-4 : 1e-9;
    const double yeps = SI().zIsTime() ? 1e-9 : 1e-4;
    Vel::resampleContinuousData( yarr, xarr, zvals_out, xeps, yeps, res );
}
